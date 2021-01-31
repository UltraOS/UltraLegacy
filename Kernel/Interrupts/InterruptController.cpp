#include "InterruptController.h"
#include "APIC.h"
#include "Core/CPU.h"
#include "LAPIC.h"
#include "MP.h"
#include "Memory/MemoryManager.h"
#include "PIC.h"
#include "ACPI/ACPI.h"

namespace kernel {

InterruptController* InterruptController::s_instance;
SMPData* InterruptController::s_smp_data;

void InterruptController::discover_and_setup()
{
    s_smp_data = ACPI::the().generate_smp_data();

    if (!s_smp_data)
        s_smp_data = MP::parse();

    if (!s_smp_data) {
        log() << "InterruptController: No APIC support detected, reverting to PIC";
        s_instance = new PIC;
        return;
    }

    log() << "InteruptController: Initializing LAPIC and IOAPIC...";

    s_instance = new APIC;
}

InterruptController& InterruptController::the()
{
    ASSERT(s_instance != nullptr);

    return *s_instance;
}
}
