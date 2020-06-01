#include "Core/Types.h"
#include "Core/Logger.h"
#include "Core/Runtime.h"
#include "Interrupts/IDT.h"
#include "Interrupts/ISR.h"
#include "Interrupts/IRQManager.h"
#include "Interrupts/PIC.h"
#include "Interrupts/PIT.h"
#include "Memory/PhysicalMemory.h"

namespace kernel {

    void run(MemoryMap memory_map)
    {
        for (u16 i = 0; i < memory_map.entry_count; ++i)
        {
            auto& entry = memory_map.entries[i];

            log() << "MEMORY -- start:" << format::as_hex
                  << static_cast<ptr_t>(entry.base_address)
                  << " size:" << static_cast<ptr_t>(entry.length)
                  << " type: "
                  << (entry.type == region_type::FREE ? "FREE" : "RESERVED");
        }

        runtime::init_global_objects();

        cli();
        PIT timer;
        ISR::install();
        IRQManager::the().install();
        IDT::the().install();
        sti();

        log() << "Hello from the kernel! " << format::as_hex << 0xDEADBEEF;

        for(;;);
    }
}
