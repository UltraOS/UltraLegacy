#include "PCI.h"
#include "Access.h"

namespace kernel {

Access* PCI::s_access;
PCI PCI::s_instance;
DynamicArray<PCI::autodetect_handler_t>* PCI::s_autodetect_handlers;

PCI::Device::Device(PCI::Location location)
    : m_location(location)
{
}

PCI::Device::BAR PCI::Device::bar(u8 bar)
{
    auto offset = base_bar_offset + (bar * PCI::register_width);
    auto raw = access().read32(location(), offset);

    static constexpr u8 io_bar_bit = 0b1;
    bool is_io = raw & io_bar_bit;

    if (is_io) {
        auto io_address = raw & ~0b11u;
        return { BAR::Type::IO, raw & io_address };
    } else {
        static constexpr u8 bar_64_bit = 0b100;
        auto is_64_bit = raw & bar_64_bit;
        Address base = raw & (~0b1111u);

#ifdef ULTRA_32
        if (is_64_bit) {
            String error;
            error << "Device " << name() << " @ " << location() << " has 64 bit bar " << bar;
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
