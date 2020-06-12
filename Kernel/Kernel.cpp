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
#include "Memory/PhysicalMemory.h"
#include "Memory/HeapAllocator.h"

namespace kernel {

    void run(MemoryMap memory_map)
    {
        runtime::ensure_loaded_correctly();

        HeapAllocator::initialize();

        u64 total_free_memory = 0;

        for (const auto& entry : memory_map)
        {
            total_free_memory += entry.length;

            log() << entry;
        }

        log() << "Total free memory: " << bytes_to_megabytes(total_free_memory) << " MB";

        runtime::init_global_objects();

        cli();
        GDT::the().create_basic_descriptors();
        GDT::the().install();
        new PIT;
        ISR::install();
        IRQManager::the().install();
        IDT::the().install();
        sti();

        for(;;);
    }
}
