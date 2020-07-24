#include "IOAPIC.h"
#include "Memory/AddressSpace.h"

namespace kernel {
Address IOAPIC::s_base;
u8      IOAPIC::s_cached_redirection_entry_count;

void IOAPIC::set_base_address(Address physical_base)
{
    // TODO: this is supposed to be 8 bytes, not a full page. replace with 8 once we support allocate_aligned
    s_base = AddressSpace::of_kernel().allocator().allocate_range(4096).begin();

    AddressSpace::of_kernel().map_page(s_base, physical_base);
}

u32 IOAPIC::redirection_entry_count()
{
    // omitting the volatile here reads bogus values
    volatile auto value = read_register(Register::VERSION);

    static constexpr auto redirection_entry_mask = 0x00FF0000;

    return ((value & redirection_entry_mask) >> 16) - 1;
}

IOAPIC::Register IOAPIC::redirection_entry(u8 index, bool is_lower)
{
    return static_cast<Register>(static_cast<u32>(Register::REDIRECTION_TABLE) + (index * 2) + (is_lower ? 0 : 1));
}

void IOAPIC::map_irq(const InterruptController::IRQInfo& irq, u8 to_index)
{
    RedirectionEntry re {};
    re.index            = to_index;
    re.delivery_mode    = DeliveryMode::FIXED;
    re.destination_mode = DestinationMode::PHYSICAL;
    re.pin_polarity     = irq.is_active_high ? PinPolarity::ACTIVE_HIGH : PinPolarity::ACTIVE_LOW;
    re.trigger_mode     = irq.is_edge ? TriggerMode::EDGE : TriggerMode::LEVEL;
    re.is_disabled      = false;
    re.local_apic_id    = InterruptController::smp_data().bootstrap_processor_apic_id;

    volatile auto* redirection_lower = Address(&re).as_pointer<u32>();

    write_register(redirection_entry(irq.redirected_irq_index, true), *redirection_lower);
    write_register(redirection_entry(irq.redirected_irq_index, false), *(redirection_lower + 1));
}

void IOAPIC::select_register(Register reg)
{
    *Address(s_base).as_pointer<volatile Register>() = reg;
}

u32 IOAPIC::read_register(Register reg)
{
    select_register(reg);

    return *Address(s_base + default_register_alignment).as_pointer<volatile u32>();
}

void IOAPIC::write_register(Register reg, u32 data)
{
    select_register(reg);

    *Address(s_base + default_register_alignment).as_pointer<volatile u32>() = data;
}

}
