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
[[noreturn]] void process_with_windows();

[[noreturn]] void run(LoaderContext* context)
{
    Logger::initialize();
    runtime::ensure_loaded_correctly();
    runtime::KernelSymbolTable::parse_all();

    // Generates native memory map, initializes BootAllocator, initializes kernel heap
    MemoryManager::early_initialize(context);

    runtime::init_global_objects();

    GDT::the().create_basic_descriptors();
    GDT::the().install();

    ExceptionDispatcher::install();
    IRQManager::install();
    SyscallDispatcher::install();
    IPICommunicator::install();
    IDT::the().install();

    MemoryManager::inititalize();

    InterruptController::discover_and_setup();
    Timer::discover_and_setup();
    CPU::initialize();

    VideoDevice::discover_and_setup(context);

    Scheduler::inititalize();

    CPU::start_all_processors();

    Time::initialize();

    WindowManager::initialize();

    PS2Controller::initialize();

    Process::create_supervisor(process_with_windows, "demo window"_sv);

    DemoTTY::initialize();

    Process::create_supervisor(dummy_kernel_process, "sleeping dummy"_sv);

    Interrupts::enable();

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
            LOCK_GUARD(window->event_queue_lock());
            window->event_queue().clear();
        }
        
        sleep::for_milliseconds(100);
    }
}

void dummy_kernel_process()
{
    for (;;) {
        sleep::for_seconds(1);
    }
}
}
