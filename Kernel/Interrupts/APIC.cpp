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
    LAPIC::initialize_for_this_processor();
    IOAPIC::set_base_address(smp_data().ioapic_address);
}

void APIC::end_of_interrupt(u8)
{
    LAPIC::end_of_interrupt();
}

void APIC::clear_all() { }

void APIC::enable_irq(u8 index)
{
    for (const auto& irq : smp_data().irqs) {
        if (irq.original_irq_index == index) {
            IOAPIC::map_irq(irq, IRQManager::irq_base_index + index);
            return;
        }
    }

    String error_string;
    error_string << "Couldn't find an irq source for " << index;
    runtime::panic(error_string.data());
}

void APIC::disable_irq(u8 index)
{
    (void)index;
}

bool APIC::is_spurious(u8 request_number)
{
    return request_number == LAPIC::spurious_irq_index;
}

void APIC::handle_spurious_irq(u8)
{
    // do nothing for APIC, it doesn't need an EOI
}
}
