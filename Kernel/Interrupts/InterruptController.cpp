#include "InterruptController.h"
#include "Core/CPU.h"
#include "LAPIC.h"
#include "PIC.h"

namespace kernel {

InterruptController* InterruptController::s_instance;

void InterruptController::discover_and_setup()
{
    if (CPU::supports_smp()) {
        log() << "InteruptController: SMP support detected. Initializing LAPIC and IOAPIC...";
        PIC::ensure_disabled();
        LAPIC::set_base_address(CPU::smp_data().local_apic_address);
        LAPIC::initialize_for_this_processor();
        // s_instance = new IOAPIC;
    } else {
        log() << "InterruptController: No SMP support detected, reverting to PIC";
        s_instance = new PIC;
    }
}

InterruptController& InterruptController::the()
{
    ASSERT(s_instance != nullptr);

    return *s_instance;
}
}
