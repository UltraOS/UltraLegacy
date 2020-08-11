#include "Common/Conversions.h"
#include "Common/Logger.h"
#include "Common/Types.h"
#include "Core/CPU.h"
#include "Core/GDT.h"
#include "Core/Runtime.h"
#include "Interrupts/ExceptionDispatcher.h"
#include "Interrupts/IDT.h"
#include "Interrupts/IPICommunicator.h"
#include "Interrupts/IRQManager.h"
#include "Interrupts/InterruptController.h"
#include "Interrupts/SyscallDispatcher.h"
#include "Interrupts/Timer.h"
#include "Memory/AddressSpace.h"
#include "Memory/HeapAllocator.h"
#include "Memory/MemoryManager.h"
#include "Memory/PhysicalMemory.h"
#include "Multitasking/Scheduler.h"
#include "Multitasking/Sleep.h"
#include "Time/RTC.h"

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

        sleep::for_seconds(1);
    }
}

void userland_process()
{
    char user_string[] = { "syscall test" };

    while (true) {
        asm("int $0x80" : : "a"(1), "b"(user_string) : "memory");
        asm("int $0x80" : : "a"(0), "b"(99) : "memory");
    }
}

void run(MemoryMap* memory_map)
{
    runtime::ensure_loaded_correctly();

    HeapAllocator::initialize();

    runtime::init_global_objects();

    MemoryManager::inititalize(*memory_map);

    AddressSpace::inititalize();

    InterruptController::discover_and_setup();

    GDT::the().create_basic_descriptors();
    GDT::the().install();

    ExceptionDispatcher::install();

    IRQManager::install();

    SyscallDispatcher::initialize();

    IPICommunicator::initialize();

    Timer::discover_and_setup();

    CPU::initialize();

    Scheduler::inititalize();

    IDT::the().install();

    CPU::start_all_processors();

    RTC::synchronize_system_clock();

    Interrupts::enable();

    // ---> SCHEDULER TESTING AREA
    // ----------------------------------------- //
    Process::create_supervisor(dummy_kernel_process);

    auto page = MemoryManager::the().allocate_page();
    AddressSpace::of_kernel().map_page(0x0000F000, page->address(), false);

    // yes we're literally copying a function :D
    copy_memory(reinterpret_cast<void*>(userland_process), reinterpret_cast<void*>(0x0000F000), 1024);

// x86 PM cannot modify non-active paging structures directly, whereas x86-64 can
// so for x86-64 we map it right here, whereas for 32 bit mode we map it in the AddressSpace::initialize
// as a hack to make ring 3 work without a proper loader, purely for testing purposes.
// (we also disable interrupts here so that this thread doesn't get interrupted before mapping the page)
#ifdef ULTRA_64
    {
        Interrupts::ScopedDisabler d;
        Process::create(0x0000F000)->m_page_directory->map_user_page(0x0000F000, page->address());
    }
#else
    Process::create(0x0000F000);
#endif

    // ----------------------------------------- //

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
