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
#include "Memory/HeapAllocator.h"
#include "Memory/MemoryManager.h"
#include "Memory/PageDirectory.h"
#include "Memory/PhysicalMemory.h"
#include "Multitasking/Scheduler.h"

namespace kernel {

void thread_1()
{
    static auto index  = 0;
    auto&&      logger = log();

    while (1)
        logger << "\r thread 1: " << ++index;
}

void run(MemoryMap memory_map)
{
    runtime::ensure_loaded_correctly();

    HeapAllocator::initialize();

    MemoryManager::inititalize(memory_map);

    runtime::init_global_objects();

    PageDirectory::inititalize();

    cli();
    GDT::the().create_basic_descriptors();
    GDT::the().install();
    new PIT;
    ISR::install();
    IRQManager::the().install();
    IDT::the().install();

    Scheduler::inititalize();

    auto   r = PageDirectory::current().allocator().allocate_range(4096);
    Thread t(&PageDirectory::current(), r.end(), reinterpret_cast<ptr_t>(thread_1));
    Scheduler::the().add_task(t);
    sti();

    static auto index  = 0;
    auto&&      logger = log();

    while (1)
        logger << "\r main thread: " << ++index;
}
}
