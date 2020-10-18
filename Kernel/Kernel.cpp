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
#include "Memory/PhysicalMemory.h"
#include "Multitasking/Scheduler.h"
#include "Multitasking/Sleep.h"
#include "Time/RTC.h"
#include "WindowManager/Painter.h"
#include "WindowManager/WindowManager.h"

namespace kernel {

void dummy_kernel_process();
void userland_process();
void process_with_windows();

void run(Context* context)
{
    runtime::ensure_loaded_correctly();

    HeapAllocator::initialize();

    Logger::initialize();

    runtime::parse_kernel_symbols();
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
    MemoryManager::inititalize(context->memory_map);
    AddressSpace::inititalize();

    InterruptController::discover_and_setup();
    Timer::discover_and_setup();
    CPU::initialize();

    VideoDevice::discover_and_setup(context->video_mode);

    Scheduler::inititalize();

    PS2Controller::initialize();

    CPU::start_all_processors();

    RTC::synchronize_system_clock();

    WindowManager::initialize();

    Interrupts::enable();

    Process::create_supervisor(process_with_windows);

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
    auto window_1 = Window::create(*Thread::current(), { 10, 10, 200, 100 });
    auto window_2 = Window::create(*Thread::current(), { 200, 200, 500, 300 });

    Painter window_1_painter(&window_1->surface());
    Painter window_2_painter(&window_2->surface());

    window_1_painter.fill_rect(window_1->view_rect(), { 0x16, 0x21, 0x3e });
    window_2_painter.fill_rect(window_2->view_rect(), { 0x0f, 0x34, 0x60 });

    {
        LockGuard lock_guard(window_1->lock());
        window_1->set_invalidated(true);
    }

    {
        LockGuard lock_guard(window_2->lock());
        window_2->set_invalidated(true);
    }

    while (true)
        ;
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
        asm("int $0x80" : : "a"(1), "b"(user_string) : "memory");
        asm("int $0x80" : : "a"(0), "b"(99) : "memory");
    }
}
}
