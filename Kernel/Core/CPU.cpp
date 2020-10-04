#include "GDT.h"

#include "Interrupts/IDT.h"
#include "Interrupts/InterruptController.h"
#include "Interrupts/LAPIC.h"

#include "Memory/MemoryManager.h"

#include "Multitasking/Process.h"
#include "Multitasking/TSS.h"

#include "CPU.h"

namespace kernel {

Atomic<size_t>               CPU::s_alive_counter;
DynamicArray<CPU::LocalData> CPU::s_processors;

void CPU::initialize()
{
    if (supports_smp()) {
        // emplace the bsp ID
        s_processors.emplace(LAPIC::my_id());
        LAPIC::timer().initialize_for_this_processor();
    } else
        s_processors.emplace(0);

    ++s_alive_counter;
}

CPU::FLAGS CPU::flags()
{
    FLAGS flags;
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
        s_processors.emplace(processor_id);

    for (auto processor_id: InterruptController::smp_data().application_processor_apic_ids) {
        size_t old_alive_counter = s_alive_counter;
        LAPIC::start_processor(processor_id);
        while (old_alive_counter == s_alive_counter)
            ;
    }
}

CPU::LocalData& CPU::current()
{
    volatile u32 this_cpu;
    if (supports_smp())
        this_cpu = LAPIC::my_id();
    else
        this_cpu = 0;

    for (auto& processor: s_processors) {
        if (processor.id() == this_cpu)
            return processor;
    }

    StackStringBuilder error_string;
    error_string << "CPU: Couldn't find the local data for cpu " << this_cpu;
    runtime::panic(error_string.data());
}

void CPU::ap_entrypoint()
{
    GDT::the().install();
    CPU::current().set_tss(new TSS);
    IDT::the().install();
    LAPIC::initialize_for_this_processor();
    LAPIC::timer().initialize_for_this_processor();
    Process::inititalize_for_this_processor();

    ++s_alive_counter;

    Interrupts::enable();

    for (;;)
        hlt();
}
}
