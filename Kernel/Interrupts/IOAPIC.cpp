#include "IOAPIC.h"
#include "Memory/AddressSpace.h"
#include "Memory/MemoryManager.h"
#include "InterruptController.h"

namespace kernel {

void IOAPIC::initialize_all()
{
    for (auto& ioapic : InterruptController::smp_data().ioapics) {
#ifdef ULTRA_32
        auto region = MemoryManager::the().allocate_kernel_non_owning("IOAPIC"_sv, Range(ioapic.address, 8));
        region->make_eternal();
        ioapic.address = region->virtual_range().begin();
#elif defined(ULTRA_64)
        ioapic.address = MemoryManager::physical_to_virtual(ioapic.address);
#endif

        auto redirections = redirection_entry_count(ioapic.address);
        ioapic.gsi_range.set_length(redirections);

        for (auto& nmi : InterruptController::smp_data().ioapic_nmis) {
            if (ioapic.gsi_range.contains(nmi.gsi))
                apply_nmi(ioapic, nmi);
        }

        log() << "IOAPIC: id " << ioapic.id << " gsi range " << ioapic.gsi_range;
    }
}

void IOAPIC::apply_nmi(const IOAPICInfo& ioapic, const IOAPICNMI& nmi)
{
    RedirectionEntry re {};
    re.delivery_mode = DeliveryMode::NMI;
    re.destination_mode = DestinationMode::PHYSICAL;
    re.pin_polarity = nmi.polarity == Polarity::ACTIVE_HIGH ? PinPolarity::ACTIVE_HIGH : PinPolarity::ACTIVE_LOW;
    re.trigger_mode = nmi.trigger_mode == decltype(nmi.trigger_mode)::EDGE ? TriggerMode::EDGE : TriggerMode::LEVEL;
    re.is_disabled = false;
    re.lapic_id = InterruptController::smp_data().bsp_lapic_id;

    re.apply_to(ioapic.address, ioapic.rebase_gsi(nmi.gsi));
}

const IOAPICInfo& IOAPIC::responsible_for_gsi(u32 gsi)
{
    for (auto& ioapic : InterruptController::smp_data().ioapics) {
        if (ioapic.gsi_range.contains(gsi))
            return ioapic;
    }

    String error_str;
    error_str << "IOAPIC: Couldn't find IOAPIC responsible for GSI " << gsi;
    runtime::panic(error_str.c_string());
}

u32 IOAPIC::redirection_entry_count(Address address)
{
    // omitting the volatile here reads bogus values
    volatile auto value = read_register(address, Register::VERSION);

    static constexpr auto redirection_entry_mask = 0x00FF0000;

    return ((value & redirection_entry_mask) >> 16) + 1;
}

void IOAPIC::RedirectionEntry::apply_to(Address address, u8 irq_index)
{
    u32 redirection_entry_as_u32[2];
    copy_memory(this, redirection_entry_as_u32, sizeof(*this));

    auto lower_register = static_cast<Register>(static_cast<u32>(Register::REDIRECTION_TABLE) + irq_index * 2);
    auto higher_register = static_cast<Register>(static_cast<u32>(lower_register) + 1);

    write_register(address, lower_register, redirection_entry_as_u32[0]);
    write_register(address, higher_register, redirection_entry_as_u32[1]);
}

void IOAPIC::map_irq(const IRQ& irq, u8 to_index)
{
    RedirectionEntry re {};
    re.index = to_index;
    re.delivery_mode = DeliveryMode::FIXED;
    re.destination_mode = DestinationMode::PHYSICAL;
    re.pin_polarity = irq.polarity == Polarity::ACTIVE_HIGH ? PinPolarity::ACTIVE_HIGH : PinPolarity::ACTIVE_LOW;
    re.trigger_mode = irq.trigger_mode == decltype(irq.trigger_mode)::EDGE ? TriggerMode::EDGE : TriggerMode::LEVEL;
    re.is_disabled = false;
    re.lapic_id = InterruptController::smp_data().bsp_lapic_id;

    auto& ioapic = responsible_for_gsi(irq.gsi);
    re.apply_to(ioapic.address, ioapic.rebase_gsi(irq.gsi));
}

void IOAPIC::select_register(Address address, Register reg)
{
    *address.as_pointer<volatile Register>() = reg;
}

u32 IOAPIC::read_register(Address address, Register reg)
{
    select_register(address, reg);

    return *Address(address + default_register_alignment).as_pointer<volatile u32>();
}

void IOAPIC::write_register(Address address, Register reg, u32 data)
{
    select_register(address, reg);

    *Address(address + default_register_alignment).as_pointer<volatile u32>() = data;
}

}
