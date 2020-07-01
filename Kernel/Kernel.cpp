#include "Common/Conversions.h"
#include "Common/Logger.h"
#include "Common/Types.h"
#include "Core/GDT.h"
#include "Core/Runtime.h"
#include "Interrupts/IDT.h"
#include "Interrupts/IRQManager.h"
#include "Interrupts/ISR.h"
#include "Interrupts/PIC.h"
#include "Interrupts/PIT.h"
#include "Interrupts/SyscallDispatcher.h"
#include "Memory/HeapAllocator.h"
#include "Memory/MemoryManager.h"
#include "Memory/PageDirectory.h"
#include "Memory/PhysicalMemory.h"
#include "Multitasking/Scheduler.h"

namespace kernel {

void dummy_kernel_process()
{
    static auto           cycles = 0;
    static constexpr u8   color  = 0xE;
    static constexpr auto column = 4;

    for (;;) {
        auto offset = vga_log("Second process: working... [", column, 0, color);

        static constexpr size_t number_length = 21;
        char                    number[number_length];

        if (to_string(++cycles, number, number_length))
            offset = vga_log(number, column, offset, color);

        vga_log("]", 4, offset, color);
    }
}

void userland_process()
{
    char user_string[] = { "syscall test" };

    while (true)
    {
        asm ("int $0x80"
            :
            : "a"(1), "S"(user_string)
            : "memory");
    }
}

void run(MemoryMap memory_map)
{
    runtime::ensure_loaded_correctly();

    HeapAllocator::initialize();

    MemoryManager::inititalize(memory_map);

    runtime::init_global_objects();

    PageDirectory::inititalize();

    InterruptDisabler::increment();

    GDT::the().create_basic_descriptors();
    GDT::the().install();
    ISR::install();
    IRQManager::the().install();
    SyscallDispatcher::initialize();
    IDT::the().install();

    Scheduler::inititalize();

    // TESTING AREA
    // ----------------------------------------- //
    Process::create_supervisor(dummy_kernel_process);

    auto page = MemoryManager::the().allocate_page();
    PageDirectory::of_kernel().map_page(0x0000F000, page->address(), false);
    // yes we're literally copiying a function :D
    copy_memory(reinterpret_cast<void*>(userland_process), reinterpret_cast<void*>(0x0000F000), 1028);

    Process::create(0x0000F000);
    // ----------------------------------------- //

    new PIT;

    InterruptDisabler::decrement();

    static auto           cycles = 0;
    static constexpr u8   color  = 0x4;
    static constexpr auto row    = 3;

    for (;;) {
        auto offset = vga_log("Main process: working... [", row, 0, color);

        static constexpr size_t number_length = 21;
        char                    number[number_length];

        if (to_string(++cycles, number, number_length))
            offset = vga_log(number, row, offset, color);

        vga_log("]", row, offset, color);
    }
}
}
