#include "Common/Conversions.h"
#include "Common/Logger.h"
#include "Common/Types.h"
#include "Core/Boot.h"
#include "Core/CPU.h"
#include "Core/GDT.h"
#include "Core/Runtime.h"
#include "Drivers/PS2/PS2Controller.h"
#include "Drivers/Video/VideoDevice.h"
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
#include "Memory/MemoryMap.h"
#include "Multitasking/Scheduler.h"
#include "Multitasking/Sleep.h"
#include "Time/RTC.h"
#include "WindowManager/DemoTTY.h"
#include "WindowManager/Painter.h"
#include "WindowManager/WindowManager.h"

namespace kernel {

[[noreturn]] void dummy_kernel_process();
[[noreturn]] void userland_process();
[[noreturn]] void process_with_windows();

void run(LoaderContext* context)
{
    Logger::initialize();
    runtime::ensure_loaded_correctly();
    runtime::parse_kernel_symbols();

    // Generates native memory map, initializes BootAllocator, initializes kernel heap
    MemoryManager::early_initialize(context);
    
    runtime::init_global_objects();

    GDT::the().create_basic_descriptors();
    GDT::the().install();

    // IDT-related stuff
    ExceptionDispatcher::install();
    IRQManager::install();
    SyscallDispatcher::install();
    IPICommunicator::install();
    IDT::the().install();

    // Memory-related stuff
    MemoryManager::inititalize();
    AddressSpace::inititalize();

    InterruptController::discover_and_setup();
    Timer::discover_and_setup();
    CPU::initialize();

    VideoDevice::discover_and_setup(context);

    Scheduler::inititalize();

    CPU::start_all_processors();

    RTC::synchronize_system_clock();

    WindowManager::initialize();

    Interrupts::enable();

    PS2Controller::initialize();

    Process::create_supervisor(process_with_windows);

    DemoTTY::initialize();

    // ---> SCHEDULER TESTING AREA
    // ----------------------------------------- //
    Process::create_supervisor(dummy_kernel_process);

    auto page = MemoryManager::the().allocate_page();
    AddressSpace::of_kernel().map_page(0x0000F000, page->address(), false);

    // yes we're literally copying a function :D
    copy_memory(reinterpret_cast<void*>(userland_process), reinterpret_cast<void*>(0x0000F000), 1024);

// The x86 kernel cannot modify non-active paging structures directly, whereas x86-64 can
// so for x86-64 we map it right here, whereas for 32 bit mode we map it in the AddressSpace::initialize
// as a hack to make ring 3 work without a proper loader, purely for testing purposes.
#ifdef ULTRA_64
    auto process = Process::create(0x0000F000, false);
    process->address_space().map_user_page(0x0000F000, page->address());
    process->commit();
#else
    Process::create(0x0000F000);
#endif
    // ----------------------------------------- //

    for (;;)
        hlt();
}

void process_with_windows()
{
    auto window = Window::create(*Thread::current(), { 10, 10, 200, 100 }, WindowManager::the().active_theme(), "Demo"_sv);
    Painter window_painter(&window->surface());

    window_painter.fill_rect(window->view_rect(), { 0x16, 0x21, 0x3e });

    window->invalidate_part_of_view_rect({ 0, 0, 200, 100 });

    while (true) {
        {
            LockGuard lock_guard(window->event_queue_lock());
            window->event_queue().clear();
        }
        sleep::for_milliseconds(10);
    }
}

void dummy_kernel_process()
{
    for (;;) {
        sleep::for_seconds(1);
    }
}

void userland_process()
{
    char user_string[] = { "syscall test" };

    while (true) {
        asm("int $0x80"
            :
            : "a"(1), "b"(user_string)
            : "memory");

        asm("int $0x80"
            :
            : "a"(0), "b"(99)
            : "memory");
    }
}
}
