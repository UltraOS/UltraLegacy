#include "PCI.h"
#include "Access.h"
#include "Interrupts/InterruptController.h"
#include "Memory/TypedMapping.h"

namespace kernel {

Access* PCI::s_access;
PCI PCI::s_instance;
DynamicArray<PCI::autodetect_handler_t>* PCI::s_autodetect_handlers;

PCI::Device::Device(const PCI::DeviceInfo& info)
    : m_info(info)
{
}

PCI::Device::BAR PCI::Device::bar(u8 bar)
{
    auto original_cr = set_command_register(MemorySpaceMode::DISABLED, IOSpaceMode::DISABLED);

    auto offset = base_bar_offset + (bar * PCI::register_width);
    auto raw = access().read32(location(), offset);

    if (raw == 0x00000000)
        return {};

    static constexpr u8 io_bar_bit = 0b1;
    bool is_io = raw & io_bar_bit;

    u32 bar_mask = is_io ? ~0b1 : ~0b1111;

    static constexpr u8 bar_64_bit = 0b100;
    auto is_64_bit = !is_io && (raw & bar_64_bit);

    BAR out_bar {};
    out_bar.address = raw & bar_mask;

    u64 upper_raw = 0;

    if (is_64_bit) {
        upper_raw = access().read32(location(), offset + PCI::register_width);
#ifdef ULTRA_64
        out_bar.address |= upper_raw << 32;
#elif defined(ULTRA_32)
        out_bar.address_upper = upper_raw;
#endif
    }

    // retrieve the address range for this bar
    access().write32(location(), offset, 0xFFFFFFFF);
    if (is_64_bit)
        access().write32(location(), offset + PCI::register_width, 0xFFFFFFFF);

    auto raw_range = access().read32(location(), offset);
    u32 bar_span_lower = raw_range & bar_mask;

    if (is_64_bit) {
        u32 bar_span_upper = access().read32(location(), offset + PCI::register_width);
        u64 bar_span_combined = static_cast<u64>(bar_span_upper) << 32 | bar_span_lower;
        bar_span_combined = ~bar_span_combined + 1;
#ifdef ULTRA_64
        out_bar.span = bar_span_combined;
#elif defined(ULTRA_32)
        out_bar.span = bar_span_combined & 0xFFFFFFFF;
        out_bar.span_upper = bar_span_combined >> 32;
#endif
    } else {
        out_bar.span = ~bar_span_lower + 1;
    }

    // restore the initial value
    access().write32(location(), offset, raw);

    if (is_64_bit)
        access().write32(location(), offset + PCI::register_width, upper_raw);

    out_bar.type = is_io ? BAR::Type::IO : (is_64_bit ? BAR::Type::MEMORY64 : BAR::Type::MEMORY32);

    set_command_register(original_cr.memory_space, original_cr.io_space);

    return out_bar;
}

#ifdef ULTRA_32
bool PCI::Device::can_use_bar(const BAR& bar)
{
    if (bar.type != BAR::Type::MEMORY64)
        return true;

    if (bar.address_upper || bar.span_upper)
        return false;

    Address64 combined_address = bar.address.raw();
    combined_address += bar.span;

    return combined_address <= (MemoryManager::max_memory_address - (Page::size - 1));
}
#endif

PCI::Device::CommandRegister PCI::Device::set_command_register(MemorySpaceMode memory_space, IOSpaceMode io_space, BusMasterMode bus_master) {
    static constexpr u16 io_space_bit = SET_BIT(0);
    static constexpr u16 memory_space_bit = SET_BIT(1);
    static constexpr u16 bus_master_bit = SET_BIT(2);

    CommandRegister original {};

    auto raw = access().read16(location(), PCI::command_register_offset);
    original.io_space = (raw & io_space_bit) ? IOSpaceMode::ENABLED : IOSpaceMode::DISABLED;
    original.memory_space = (raw & memory_space_bit) ? MemorySpaceMode::ENABLED : MemorySpaceMode::DISABLED;
    original.bus_master = (raw & bus_master_bit) ? BusMasterMode::ENABLED : BusMasterMode::DISABLED;

    if (io_space == IOSpaceMode::DISABLED)
        raw &= ~io_space_bit;
    else if (io_space == IOSpaceMode::ENABLED)
        raw |= io_space_bit;

    if (memory_space == MemorySpaceMode::DISABLED)
        raw &= ~memory_space_bit;
    else if (memory_space == MemorySpaceMode::ENABLED)
        raw |= memory_space_bit;

    if (bus_master == BusMasterMode::DISABLED)
        raw &= ~bus_master_bit;
    else if (bus_master == BusMasterMode::ENABLED)
        raw |= bus_master_bit;

    access().write16(location(), PCI::command_register_offset, raw);

    return original;
}

void PCI::Device::enable_msi(u16 vector)
{
    auto cap = linear_search_for(m_info.capabilities.begin(), m_info.capabilities.end(),
        [](const PCI::DeviceInfo::Capability& cap) {
            return cap.id == PCI::DeviceInfo::Capability::ID::MESSAGE_SIGNALED_INTERRUPTS || cap.id == PCI::DeviceInfo::Capability::ID::MESSAGE_SIGNALED_INTERRUPTS_EXTENDED;
        });

    ASSERT(cap != m_info.capabilities.end());

    static constexpr u32 message_control_offset = 0x2;
    static constexpr u32 destination_id_bit_offset = 12;

    // As per intel manual: "Bits 31-20 - These bits contain a fixed value for interrupt messages (0FEEh)"
    // Also, redirection hint 0 (use destination ID), destination mode 0 (ignored when RH is 0)
    u32 message_address = 0xFEE00000 | (InterruptController::smp_data().bsp_lapic_id << destination_id_bit_offset);

    // Edge sensitive (0) + Level Assert (1) but we don't have to set it because the spec says:
    // "For edge triggered interrupts this field is not used."
    u16 message_data = vector;

    auto message_control = access().read16(location(), cap->offset + message_control_offset);

    if (cap->id == PCI::DeviceInfo::Capability::ID::MESSAGE_SIGNALED_INTERRUPTS) {
        static constexpr u32 message_address_offset = 0x4;
        static constexpr u16 bit64_support = SET_BIT(7);
        static constexpr u16 single_message_mask = 0b1111111110001111;
        static constexpr u16 msi_enable_bit = SET_BIT(0);

        message_control &= single_message_mask;
        message_control |= msi_enable_bit;

        u32 message_data_offset = (message_control & bit64_support) ? 0xC : 0x8;

        access().write32(location(), cap->offset + message_address_offset, message_address);
        access().write16(location(), cap->offset + message_data_offset, message_data);
        access().write16(location(), cap->offset + message_control_offset, message_control);
    } else { // MSI-X
        static constexpr u16 msi_enable_bit = SET_BIT(15);
        static constexpr u16 bir_offset = 0x4;
        static constexpr u32 bir_mask = 0b111;

        message_control |= msi_enable_bit;

        auto raw_bir = access().read32(location(), cap->offset + bir_offset);

        auto bir = raw_bir & bir_mask;
        auto table_offset = raw_bir & ~bir_mask;

        auto table_bar = bar(bir);
        ASSERT(table_bar.type == BAR::Type::MEMORY32);

        auto table_address = table_bar.address + table_offset;
        auto mapped_table = TypedMapping<u32>::create("MSI-X Table"_sv, table_address, 4 * sizeof(u32));

        mapped_table[0] = message_address;
        mapped_table[2] = message_data;
        mapped_table[3] = 0; // unmasked

        access().write32(location(), cap->offset + message_control_offset, message_control);
    }
}

void PCI::Device::clear_interrupts_disabled()
{
    static constexpr u16 interrupts_disabled_bit = SET_BIT(10);

    auto raw = access().read16(location(), PCI::command_register_offset);
    raw &= ~interrupts_disabled_bit;
    access().write16(location(), PCI::command_register_offset, raw);
}

u16 PCI::Device::legacy_pci_irq_line()
{
    return access().read8(location(), PCI::legacy_irq_line_offset);
}

Access& PCI::access()
{
    ASSERT(s_access != nullptr);
    return *s_access;
}

void PCI::collect_all_devices()
{
    s_access = Access::detect();

    if (!s_access)
        return;

    m_devices = access().list_all_devices();
}

void PCI::initialize_supported()
{
    if (!s_autodetect_handlers)
        return;

    auto& devices = PCI::the().devices();

    for (auto handler : *s_autodetect_handlers)
        handler(devices);
}

void PCI::register_autodetect_handler(autodetect_handler_t handler)
{
    if (!s_autodetect_handlers)
        s_autodetect_handlers = new DynamicArray<autodetect_handler_t>();

    s_autodetect_handlers->append(handler);
}

}
