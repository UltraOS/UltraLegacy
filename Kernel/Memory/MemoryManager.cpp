#include "Common/Logger.h"

#include "Interrupts/PageFault.h"
#include "Interrupts/Utilities.h"

#include "Multitasking/Scheduler.h"

#include "AddressSpace.h"
#include "MemoryManager.h"
#include "Page.h"
#include "PhysicalRegion.h"

// #define MEMORY_MANAGER_DEBUG

namespace kernel {

MemoryManager* MemoryManager::s_instance;

MemoryManager::MemoryManager(const MemoryMap& memory_map)
{
    m_memory_map = &memory_map;

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

#ifdef ULTRA_32
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
#endif

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
    log() << "MemoryManager: Total free memory: " << bytes_to_megabytes(total_free_memory) << " MB ("
          << total_free_memory / Page::size << " pages) ";
#endif
}

#ifdef ULTRA_32
u8* MemoryManager::quickmap_page(Address physical_address)
{
#ifdef MEMORY_MANAGER_DEBUG
    log() << "MemoryManager: quickmapping vaddr " << m_quickmap_range.begin() << " to " << physical_address;
#endif

    AddressSpace::current().map_page(m_quickmap_range.begin(), physical_address);

    return m_quickmap_range.as_pointer<u8>();
}

u8* MemoryManager::quickmap_page(const Page& page)
{
    return quickmap_page(page.address());
}

void MemoryManager::unquickmap_page()
{
    AddressSpace::current().unmap_page(m_quickmap_range.begin());
}
#endif

RefPtr<Page> MemoryManager::allocate_page(bool should_zero)
{
    bool interrupt_state = false;
    m_lock.lock(interrupt_state);

    for (auto& region: m_physical_regions) {
        if (!region.has_free_pages())
            continue;

        auto page = region.allocate_page();

        ASSERT(page);

#ifdef MEMORY_MANAGER_DEBUG
        log() << "MemoryManager: zeroing the page at physaddr " << page->address();
#endif

        if (should_zero) {
#ifdef ULTRA_32
            ScopedPageMapping mapping(page->address());

            zero_memory(mapping.as_pointer(), Page::size);

#elif defined(ULTRA_64)
            zero_memory(physical_to_virtual(page->address()).as_pointer<void>(), Page::size);
#endif
        }

        m_lock.unlock(interrupt_state);
        return page;
    }

    error() << "MemoryManager: Out of physical memory!";
    hang();
}

void MemoryManager::free_page(Page& page)
{
    bool interrupt_state = false;
    m_lock.lock(interrupt_state);

    for (auto& region: m_physical_regions) {
        if (region.contains(page)) {
            region.free_page(page);
            m_lock.unlock(interrupt_state);
            return;
        }
    }

    error() << "Couldn't find the region that owns the page at " << page.address();
    hang();
}

void MemoryManager::handle_page_fault(const PageFault& fault)
{
    if (AddressSpace::current().allocator().is_allocated(fault.address())) {
#ifdef MEMORY_MANAGER_DEBUG
        log() << "MemoryManager: expected page fault on core " << CPU::current().id() << fault;
#endif
        auto rounded_address = fault.address() & Page::alignment_mask;

        auto page = the().allocate_page();
        AddressSpace::current().store_physical_page(page);
        AddressSpace::current().map_page(rounded_address, page->address(), Thread::current()->is_supervisor());
    } else {
        error() << "MemoryManager: unexpected page fault on core " << CPU::current().id() << fault;
        hang();
    }
}

void MemoryManager::inititalize(AddressSpace& directory)
{
    bool interrupt_state = false;
    m_lock.lock(interrupt_state);
#ifdef ULTRA_32
    // map the directory's physical page somewhere temporarily
    ScopedPageMapping mapping(directory.physical_address());

#ifdef MEMORY_MANAGER_DEBUG
    log() << "MemoryManager: Setting up kernel mappings for the directory at physaddr " << directory.physical_address();
#endif

    // copy the kernel mappings
    // TODO: this assumes that all kernel tables are contiguous
    //       so it could backfire later.
    for (size_t i = kernel_first_table_index; i <= kernel_last_table_index; ++i) {
        auto& entry = AddressSpace::current().entry_at(i);

        if (!entry.is_present())
            break;

        directory.entry_at(i, mapping.raw()) = entry;
    }

    // TODO: remove this later when removing ring 3 tests
    directory.entry_at(0, mapping.raw()) = AddressSpace::current().entry_at(0);

    // create the recursive mapping
    directory.entry_at(recursive_entry_index, mapping.raw())
        .set_physical_address(directory.physical_address())
        .make_supervisor_present();
#elif defined(ULTRA_64)
    static constexpr size_t kernel_pdpts[] = { 256, 511 };

    for (auto pdpt_index: kernel_pdpts) {
        auto& entry = AddressSpace::of_kernel().entry_at(pdpt_index);
        directory.entry_at(pdpt_index) = entry;
    }
#endif
    m_lock.unlock(interrupt_state);
}

#ifdef ULTRA_32
void MemoryManager::set_quickmap_range(const VirtualAllocator::Range& range)
{
    m_quickmap_range = range;
}
#endif

void MemoryManager::force_preallocate(const VirtualAllocator::Range& range, bool should_zero)
{
    ASSERT_PAGE_ALIGNED(range.begin());

    log() << "MemoryManager: force allocating " << range.length() / Page::size << " physical pages...";

    for (Address address = range.begin(); address < range.end(); address += Page::size) {
        auto page = allocate_page(should_zero);
        AddressSpace::current().map_page(address, page->address());
        AddressSpace::current().store_physical_page(page);
    }
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
