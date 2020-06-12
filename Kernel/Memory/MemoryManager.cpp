#include "Common/Logger.h"
#include "MemoryManager.h"

namespace kernel {

    MemoryManager* MemoryManager::s_instance;

    MemoryManager::MemoryManager(const MemoryMap& memory_map)
    {
        u64 total_free_memory = 0;

        for (const auto& entry : memory_map)
        {
            log() << entry;

            if (entry.is_reserved())
                continue;

            auto base_address = entry.base_address;
            auto length = entry.length;

            if (base_address < 1 * MB)
            {
                log() << "MemoryManager: skipping a conventional memory entry at " << format::as_hex << base_address;
                continue;
            }
            if (base_address % page_size)
            {
                log() << "MemoryManager: got an unaligned memory map entry " << format::as_hex << base_address << " size:" << length;

                auto alignment_overhead = page_size - (base_address % page_size);

                base_address += alignment_overhead;
                length       -= alignment_overhead;

                log() << "MemoryManager: aligned address:" << format::as_hex << base_address << " size:" << length;
            }
            if (length % page_size)
            {
                log() << "MemoryManager: got an unaligned memory map entry length" << length;

                length -= length % page_size;

                log() << "MemoryManager: aligned length at " << length;
            }
            if (length < page_size)
            {
                log() << "MemoryManager: length is too small, skipping the entry (" << length << " < " << page_size << ")";
                continue;
            }

            for (auto address = base_address; address < (base_address + length); address += page_size)
            {
                // First 8 MB are reserved for the kernel
                if (address < 8 * MB)
                    continue;

                // Make use of this once we have PAE
                if (address > 4 * GB)
                    continue;

                total_free_memory += page_size;

                // save these pages somewhere
            }
        }

        log() << "Total free memory: "
              << bytes_to_megabytes_precise(total_free_memory) << " MB ("
              << total_free_memory / page_size << " pages) ";
    }

    MemoryManager& MemoryManager::the()
    {
        return *s_instance;
    }

    void MemoryManager::inititalize(const MemoryMap& memory_map)
    {
        s_instance = new MemoryManager(memory_map);
    }
}
