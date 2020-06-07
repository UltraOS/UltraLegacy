#include "Core/Types.h"
#include "Core/Logger.h"
#include "Core/Runtime.h"
#include "Core/Conversions.h"
#include "GDT.h"
#include "Interrupts/IDT.h"
#include "Interrupts/ISR.h"
#include "Interrupts/IRQManager.h"
#include "Interrupts/PIC.h"
#include "Interrupts/PIT.h"
#include "Memory/PhysicalMemory.h"
#include "Memory/Allocator.h"

class test
{
public:
    test() = default;
};

namespace kernel {

    void run(MemoryMap memory_map)
    {
        runtime::ensure_loaded_correctly();

        Allocator::initialize();

        u64 total_free_memory = 0;

        for (const auto& entry : memory_map)
        {
            total_free_memory += entry.length;

            log() << "MEMORY -- start:" << format::as_hex
                  << entry.base_address
                  << " size:" << entry.length
                  << " type: "
                  << (entry.type == region_type::FREE ? "FREE" : "RESERVED");
        }

        log() << "Total free memory: " << bytes_to_megabytes(total_free_memory) << " MB";

        runtime::init_global_objects();

        cli();
        GDT::the().create_basic_descriptors();
        GDT::the().install();
        asm volatile("xchg %bx, %bx");
        PIT timer;
        ISR::install();
        IRQManager::the().install();
        IDT::the().install();
        sti();

        for(;;);
    }
}
