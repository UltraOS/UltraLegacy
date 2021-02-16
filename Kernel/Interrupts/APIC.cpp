#include "APIC.h"
#include "Core/CPU.h"
#include "IOAPIC.h"
#include "LAPIC.h"
#include "PIC.h"

namespace kernel {

APIC::APIC()
{
    PIC::ensure_disabled();
    LAPIC::set_base_address(smp_data().lapic_address);
    new LAPIC::SpuriousHandler();
    LAPIC::initialize_for_this_processor();
    IOAPIC::initialize_all();
}

void APIC::end_of_interrupt(u8)
{
    LAPIC::end_of_interrupt();
}

void APIC::clear_all() { }

void APIC::enable_irq_for(const IRQHandler& handler)
{
    ASSERT(handler.irq_handler_type() == IRQHandler::Type::LEGACY);

    auto& irqs = smp_data().legacy_irqs_to_info;
    auto irq_index = handler.legacy_irq_number();

    if (!irqs.contains(irq_index)) {
        String error_string;
        error_string << "Couldn't find an irq source for " << irq_index;
        runtime::panic(error_string.data());
    }

    IOAPIC::map_legacy_irq(irqs.get(irq_index), handler.interrupt_vector());
}

void APIC::disable_irq_for(const IRQHandler&)
{
    // TODO
    ASSERT_NEVER_REACHED();
}

}
