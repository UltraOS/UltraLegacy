#include "Core/Runtime.h"
#include "Core/GDT.h"
#include "Common/Types.h"
#include "Common/Logger.h"
#include "Common/Conversions.h"
#include "Interrupts/IDT.h"
#include "Interrupts/ISR.h"
#include "Interrupts/IRQManager.h"
#include "Interrupts/PIC.h"
#include "Interrupts/PIT.h"
#include "Memory/HeapAllocator.h"
#include "Memory/PhysicalMemory.h"
#include "Memory/MemoryManager.h"
#include "Memory/PageDirectory.h"

namespace kernel {

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
        sti();

        // just a test thing to be removed
        log() << "TABLE 768 ENTRY 1023: address " << format::as_hex

              << PageDirectory::of_kernel().table_at(768).entry_at(1023).physical_address();

        log() << "TABLE 768 ENTRY 1023: attribs "  << PageDirectory::of_kernel().table_at(768).entry_at(1023).attributes();

        for(;;);
    }
}
