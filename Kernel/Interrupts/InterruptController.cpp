#include "InterruptController.h"
#include "ACPI/ACPI.h"
#include "APIC.h"
#include "Core/CPU.h"
#include "IRQHandler.h"
#include "LAPIC.h"
#include "MP.h"
#include "Memory/MemoryManager.h"
#include "PIC.h"

namespace kernel {

SMPData* InterruptController::s_smp_data;

bool InterruptController::is_initialized()
{
    return DeviceManager::is_initialized() && DeviceManager::the().primary_of(Category::INTERRUPT_CONTROLLER) != nullptr;
}

InterruptController::InterruptController()
    : Device(Category::INTERRUPT_CONTROLLER)
{
}

void InterruptController::discover_and_setup()
{
    s_smp_data = ACPI::the().generate_smp_data();

    if (!s_smp_data)
        s_smp_data = MP::parse();

    if (!s_smp_data) {
        log() << "InterruptController: No APIC support detected, reverting to PIC";
        new PIC;
        return;
    }

    log() << "InteruptController: Initializing LAPIC and IOAPIC...";
    (new APIC)->make_primary();
}

InterruptController& InterruptController::primary()
{
    auto* primary = DeviceManager::the().primary_of(Category::INTERRUPT_CONTROLLER);
    ASSERT(primary != nullptr);

    return *static_cast<InterruptController*>(primary);
}
}
