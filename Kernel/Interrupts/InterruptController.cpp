#include "InterruptController.h"
#include "APIC.h"
#include "Core/CPU.h"
#include "LAPIC.h"
#include "PIC.h"

namespace kernel {

InterruptController* InterruptController::s_instance;

void InterruptController::discover_and_setup()
{
    if (CPU::supports_smp()) {
        log() << "InteruptController: SMP support detected. Initializing LAPIC and IOAPIC...";
        s_instance = new APIC;
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
