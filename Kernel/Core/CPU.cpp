#include "CPU.h"
#include "Interrupts/Common.h"
#include "Interrupts/InterruptController.h"
#include "Interrupts/LAPIC.h"
#include "Memory/MemoryManager.h"

namespace kernel {

CPU::EFLAGS CPU::flags()
{
    EFLAGS flags;
    asm volatile("pushf\n"
                 "pop %0\n"
                 : "=a"(flags)::"memory");

    return flags;
}

bool CPU::supports_smp()
{
    return InterruptController::supports_smp();
}

void CPU::start_all_processors()
{
    if (!InterruptController::supports_smp())
        return;

    for (auto processor_id: InterruptController::smp_data().application_processor_apic_ids)
        LAPIC::start_processor(processor_id);
}
}
