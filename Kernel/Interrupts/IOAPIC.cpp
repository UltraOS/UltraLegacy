#include "IOAPIC.h"
#include "Memory/AddressSpace.h"
#include "Memory/MemoryManager.h"

namespace kernel {
Address IOAPIC::s_base;
u8 IOAPIC::s_cached_redirection_entry_count;

void IOAPIC::set_base_address(Address physical_base)
{
#ifdef ULTRA_32
    auto region = MemoryManager::the().allocate_kernel_non_owning("IOAPIC"_sv, Range(physical_base, 8));
    region->make_eternal();
    s_base = region->virtual_range().begin();
#elif defined(ULTRA_64)
    s_base = MemoryManager::physical_to_virtual(physical_base);
#endif
}

u32 IOAPIC::redirection_entry_count()
{
    // omitting the volatile here reads bogus values
    volatile auto value = read_register(Register::VERSION);

    static constexpr auto redirection_entry_mask = 0x00FF0000;

    return ((value & redirection_entry_mask) >> 16) - 1;
}

void IOAPIC::RedirectionEntry::apply_redirection_to(u8 irq_index)
{
    u32 redirection_entry_as_u32[2];
    copy_memory(this, redirection_entry_as_u32, sizeof(*this));

    auto lower_register = static_cast<Register>(static_cast<u32>(Register::REDIRECTION_TABLE) + irq_index * 2);
    auto higher_register = static_cast<Register>(static_cast<u32>(lower_register) + 1);

    write_register(lower_register, redirection_entry_as_u32[0]);
    write_register(higher_register, redirection_entry_as_u32[1]);
}

void IOAPIC::map_irq(const InterruptController::IRQInfo& irq, u8 to_index)
{
    RedirectionEntry re {};
    re.index = to_index;
    re.delivery_mode = DeliveryMode::FIXED;
    re.destination_mode = DestinationMode::PHYSICAL;
    re.pin_polarity = irq.is_active_high ? PinPolarity::ACTIVE_HIGH : PinPolarity::ACTIVE_LOW;
    re.trigger_mode = irq.is_edge ? TriggerMode::EDGE : TriggerMode::LEVEL;
    re.is_disabled = false;
    re.local_apic_id = InterruptController::smp_data().bootstrap_processor_apic_id;

    re.apply_redirection_to(irq.redirected_irq_index);
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
