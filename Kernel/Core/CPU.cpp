#include "GDT.h"

#include "Interrupts/IDT.h"
#include "Interrupts/InterruptController.h"
#include "Interrupts/LAPIC.h"

#include "Memory/MemoryManager.h"

#include "CPU.h"

namespace kernel {

DynamicArray<CPU::LocalData> CPU::s_processors;

void CPU::initialize()
{
    // emplace the bsp ID
    s_processors.emplace(LAPIC::my_id());
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
        LAPIC::start_processor(processor_id);
}

CPU::LocalData& CPU::current()
{
    auto this_cpu = LAPIC::my_id();

    for (auto& processor: s_processors) {
        if (processor.id() == this_cpu)
            return processor;
    }

    error() << "CPU: Couldn't find the local data for cpu " << this_cpu;
    hang();
}

void CPU::ap_entrypoint()
{
    GDT::the().install();
    IDT::the().install();
    LAPIC::initialize_for_this_processor();
    Interrupts::enable();

    auto my_id = LAPIC::my_id();
    s_processors.emplace(my_id);

    static size_t row    = 7;
    auto          my_row = row++;
    size_t        cycles = 0;

    char buf[4];
    to_string(my_id, buf, 4);

    static constexpr u8 color = 0x3;

    for (;;) {
        auto offset = vga_log("Core ", my_row, 0, color);

        offset = vga_log(buf, my_row, offset, color);
        offset = vga_log(": working... [", my_row, offset, color);

        static constexpr size_t number_length = 21;
        char                    number[number_length];

        if (to_string(++cycles, number, number_length))
            offset = vga_log(number, my_row, offset, color);

        vga_log("]", my_row, offset, color);
    }
}
}
