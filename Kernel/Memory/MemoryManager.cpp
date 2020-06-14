#include "Common/Logger.h"
#include "MemoryManager.h"
#include "PhysicalRegion.h"
#include "Page.h"

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

            // First 8 MB are reserved for the kernel
            if (base_address < (8 * MB))
            {
                if ((base_address + length) < (8 * MB))
                {
                    log() << "MemoryManager: skipping a lower memory region at " << format::as_hex << base_address;
                    continue;
                }
                else
                {
                    log() << "MemoryManager: trimming a low memory region at " << format::as_hex << base_address;

                    auto trim_overhead = 8 * MB - base_address;
                    base_address += trim_overhead;
                    length       -= trim_overhead;

                    log() << "MemoryManager: trimmed region:" << format::as_hex << base_address;
                }
            }

            // Make use of this once we have PAE
            if (base_address > (4 * GB))
            {
                log() << "MemoryManager: skipping a higher memory region at " << format::as_hex << base_address;
                continue;
            }
            else if ((base_address + length) > (4 * GB))
            {
                log() << "MemoryManager: trimming a big region (outside of 4GB) at" << format::as_hex << base_address << " length:" << length;

                length -= (base_address + length) - 4 * GB;
            }

            if (base_address % Page::size)
            {
                log() << "MemoryManager: got an unaligned memory map entry " << format::as_hex << base_address << " size:" << length;

                auto alignment_overhead = Page::size - (base_address % Page::size);

                base_address += alignment_overhead;
                length       -= alignment_overhead;

                log() << "MemoryManager: aligned address:" << format::as_hex << base_address << " size:" << length;
            }
            if (length % Page::size)
            {
                log() << "MemoryManager: got an unaligned memory map entry length" << length;

                length -= length % Page::size;

                log() << "MemoryManager: aligned length at " << length;
            }
            if (length < Page::size)
            {
                log() << "MemoryManager: length is too small, skipping the entry (" << length << " < " << Page::size << ")";
                continue;
            }

            auto& this_region = m_physical_regions.emplace(base_address, length);

            log() << "MemoryManager: A new physical region -> " << this_region;
            total_free_memory += this_region.free_pages() * Page::size;
        }

        log() << "Total free memory: "
              << bytes_to_megabytes_precise(total_free_memory) << " MB ("
              << total_free_memory / Page::size << " pages) ";
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
