#include "MemoryManager.h"
#include "Common/Logger.h"
#include "Interrupts/Common.h"
#include "Page.h"
#include "PageDirectory.h"
#include "PhysicalRegion.h"
#include "Multitasking/Scheduler.h"

#define MEMORY_MANAGER_DEBUG

namespace kernel {

MemoryManager* MemoryManager::s_instance;

MemoryManager::MemoryManager(const MemoryMap& memory_map)
{
    u64 total_free_memory = 0;

    for (const auto& entry: memory_map) {
        log() << entry;

        if (entry.is_reserved())
            continue;

        auto base_address = entry.base_address;
        auto length       = entry.length;

        if (base_address < kernel_reserved_size) {
            if ((base_address + length) < kernel_reserved_size) {
#ifdef MEMORY_MANAGER_DEBUG
                log() << "MemoryManager: skipping a lower memory region at " << format::as_hex << base_address;
#endif

                continue;
            } else {
#ifdef MEMORY_MANAGER_DEBUG
                log() << "MemoryManager: trimming a low memory region at " << format::as_hex << base_address;
#endif

                auto trim_overhead = kernel_reserved_size - base_address;
                base_address += trim_overhead;
                length -= trim_overhead;

#ifdef MEMORY_MANAGER_DEBUG
                log() << "MemoryManager: trimmed region:" << format::as_hex << base_address;
#endif
            }
        }

        // Make use of this once we have PAE
        if (base_address > max_memory_address) {
#ifdef MEMORY_MANAGER_DEBUG
            log() << "MemoryManager: skipping a higher memory region at " << format::as_hex << base_address;
#endif

            continue;
        } else if ((base_address + length) > max_memory_address) {
#ifdef MEMORY_MANAGER_DEBUG
            log() << "MemoryManager: trimming a big region (outside of 4GB) at" << format::as_hex << base_address
                  << " length:" << length;
#endif

            length -= (base_address + length) - max_memory_address;
        }

        if (!Page::is_aligned(base_address)) {
#ifdef MEMORY_MANAGER_DEBUG
            log() << "MemoryManager: got an unaligned memory map entry " << format::as_hex << base_address
                  << " size:" << length;
#endif

            auto alignment_overhead = Page::size - (base_address % Page::size);

            base_address += alignment_overhead;
            length -= alignment_overhead;

#ifdef MEMORY_MANAGER_DEBUG
            log() << "MemoryManager: aligned address:" << format::as_hex << base_address << " size:" << length;
#endif
        }
        if (!Page::is_aligned(length)) {
#ifdef MEMORY_MANAGER_DEBUG
            log() << "MemoryManager: got an unaligned memory map entry length" << length;
#endif

            length -= length % Page::size;

#ifdef MEMORY_MANAGER_DEBUG
            log() << "MemoryManager: aligned length at " << length;
#endif
        }
        if (length < Page::size) {

#ifdef MEMORY_MANAGER_DEBUG
            log() << "MemoryManager: length is too small, skipping the entry (" << length << " < " << Page::size << ")";
#endif

            continue;
        }

        auto& this_region = m_physical_regions.emplace(base_address, length);

#ifdef MEMORY_MANAGER_DEBUG
        log() << "MemoryManager: A new physical region -> " << this_region;
#endif

        total_free_memory += this_region.free_page_count() * Page::size;
    }

#ifdef MEMORY_MANAGER_DEBUG
    log() << "MemoryManager: Total free memory: " << bytes_to_megabytes_precise(total_free_memory) << " MB ("
          << total_free_memory / Page::size << " pages) ";
#endif
}

u8* MemoryManager::quickmap_page(Address physical_address)
{
#ifdef MEMORY_MANAGER_DEBUG
    log() << "MemoryManager: quickmapping vaddr " << format::as_hex << m_quickmap_range.begin() << " to "
          << physical_address;
#endif

    PageDirectory::current().map_page(m_quickmap_range.begin(), physical_address);

    return m_quickmap_range.as_pointer<u8>();
}

u8* MemoryManager::quickmap_page(const Page& page)
{
    return quickmap_page(page.address());
}

void MemoryManager::unquickmap_page()
{
    PageDirectory::current().unmap_page(m_quickmap_range.begin());
}

RefPtr<Page> MemoryManager::allocate_page()
{
    for (auto& region: m_physical_regions) {
        if (!region.has_free_pages())
            continue;

        auto page = region.allocate_page();

        ASSERT(page);

        InterruptDisabler d;

        ScopedPageMapping mapping(page->address());

#ifdef MEMORY_MANAGER_DEBUG
        log() << "MemoryManager: zeroing the page at physaddr " << format::as_hex << page->address();
#endif

        zero_memory(mapping.as_pointer(), Page::size);

        return page;
    }

    error() << "MemoryManager: Out of physical memory!";
    hang();
}

void MemoryManager::free_page(Page& page)
{
    for (auto& region: m_physical_regions) {
        if (region.contains(page)) {
            region.free_page(page);
            return;
        }
    }

    error() << "Couldn't find the region that owns the page at " << format::as_hex << page.address();
    hang();
}

void MemoryManager::handle_page_fault(const PageFault& fault)
{
    if (PageDirectory::current().allocator().is_allocated(fault.address())) {
#ifdef MEMORY_MANAGER_DEBUG
        log() << "MemoryManager: expected page fault: " << fault;
#endif

        auto rounded_address = fault.address() & Page::alignment_mask;

        auto page = the().allocate_page();
        PageDirectory::current().store_physical_page(page);
        PageDirectory::current().map_page(rounded_address, page->address(), Scheduler::current_thread().is_supervisor());
    } else {
        error() << "MemoryManager: unexpected page fault: " << fault;
        hang();
    }
}

void MemoryManager::inititalize(PageDirectory& directory)
{
    // Memory Managment TODOS:
    // 1. Fix this function                                                         --- kinda done
    // a. add handling for missing page directories                              --- kinda done
    // b. get rid of hardcoded kernel page directory entries (keep them dynamic) --- kinda done
    // 2. Add enums for all attributes for pages                                    --- kinda done
    // 3. Add a page fault handler                                                  --- kinda done
    // 4. Add Setters/Getter for all paging structures                              --- kinda done
    // 5. cli()/hlt() here somewhere?                                               --- kinda done
    // 6. Add a page invalidator instead of flushing the entire tlb                 --- kinda done
    // 7. Add allocate_aligned_range for VirtualAllocator
    // 8. A lot more debug logging for all allocators and page table related stuff --- kinda done
    // 9. Zero all new pages                                                             --- kinda done
    // maybe add some sort of quickmap virtual range where I could zero the new pages --- kinda done
    // 10. make kernel entries global                                                    --- kinda done
    // 11. tons of magic numbers                                                         --- kinda done
    // 12. more TBD...

    InterruptDisabler d;

    // map the directory's physical page somewhere temporarily
    ScopedPageMapping mapping(directory.physical_address());

#ifdef MEMORY_MANAGER_DEBUG
    log() << "MemoryManager: Setting up kernel mappings for the directory at physaddr " << format::as_hex
          << directory.physical_address();
#endif

    // copy the kernel mappings
    // TODO: this assumes that all kernel tables are contiguous
    //       so it could backfire later.
    for (size_t i = kernel_first_table_index; i <= kernel_last_table_index; ++i) {
        auto& entry = PageDirectory::current().entry_at(i);

        if (!entry.is_present())
            break;

        directory.entry_at(i, mapping.raw()) = entry;
    }

    // TODO: remove this later when removing ring 3 tests
    directory.entry_at(0, mapping.raw()) = PageDirectory::current().entry_at(0);

    // create the recursive mapping
    directory.entry_at(recursive_entry_index, mapping.raw())
        .set_physical_address(directory.physical_address())
        .make_supervisor_present();
}

void MemoryManager::set_quickmap_range(const VirtualAllocator::Range& range)
{
    m_quickmap_range = range;
}

MemoryManager& MemoryManager::the()
{
    ASSERT(s_instance != nullptr);

    return *s_instance;
}

void MemoryManager::inititalize(const MemoryMap& memory_map)
{
    s_instance = new MemoryManager(memory_map);
}
}
