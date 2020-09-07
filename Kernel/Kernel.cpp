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
#include "WindowManager/Compositor.h"

namespace kernel {

void dummy_kernel_process();
void userland_process();

void run(Context* context)
{
    runtime::ensure_loaded_correctly();

    HeapAllocator::initialize();

    Logger::initialize();

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

    Scheduler::inititalize();

    VideoDevice::discover_and_setup(context->video_mode);
    PS2Controller::initialize();

    CPU::start_all_processors();

    RTC::synchronize_system_clock();

    Interrupts::enable();

    Process::create_supervisor(&Compositor::run);

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
