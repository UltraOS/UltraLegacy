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
    auto offset = base_bar_offset + (bar * PCI::register_width);
    auto raw = access().read32(location(), offset);

    if (raw == 0x00000000)
        return { BAR::Type::NOT_PRESENT, nullptr };

    static constexpr u8 io_bar_bit = 0b1;
    bool is_io = raw & io_bar_bit;

    if (is_io) {
        auto io_address = raw & ~0b11u;
        return { BAR::Type::IO, raw & io_address };
    }

    static constexpr u8 bar_64_bit = 0b100;
    auto is_64_bit = raw & bar_64_bit;
    Address base = raw & (~0b1111u);

#ifdef ULTRA_32
    if (is_64_bit) {
        String error;
        error << "Device @ " << location() << " has 64 bit bar " << bar;
        runtime::panic(error.c_string());
    }
#endif

    if (is_64_bit) {
        offset += PCI::register_width;
        u64 upper_raw = access().read32(location(), offset);
        base += upper_raw << 32ull;
    }

    return { BAR::Type::MEMORY, base };
}

void PCI::Device::make_bus_master()
{
    static constexpr u16 bus_master_bit = SET_BIT(2);

    auto raw = access().read16(location(), PCI::command_register_offset);
    raw |= bus_master_bit;
    access().write16(location(), PCI::command_register_offset, raw);
}

void PCI::Device::enable_memory_space()
{
    static constexpr u16 memory_space_bit = SET_BIT(1);

    auto raw = access().read16(location(), PCI::command_register_offset);
    raw |= memory_space_bit;
    access().write16(location(), PCI::command_register_offset, raw);
}

void PCI::Device::enable_io_space()
{
    static constexpr u16 io_space_bit = SET_BIT(0);

    auto raw = access().read16(location(), PCI::command_register_offset);
    raw |= io_space_bit;
    access().write16(location(), PCI::command_register_offset, raw);
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
        ASSERT(table_bar.type == BAR::Type::MEMORY);

        auto table_address = table_bar.address + table_offset;
        auto mapped_table = TypedMapping<u32>::create("MSI-X Table"_sv, table_address, 4 * sizeof(u32));

        mapped_table.get()[0] = message_address;
        mapped_table.get()[2] = message_data;
        mapped_table.get()[3] = 0; // unmasked

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
