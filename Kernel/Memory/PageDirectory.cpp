#include "Common/Math.h"
#include "Core/InterruptDisabler.h"
#include "MemoryManager.h"
#include "PageTable.h"
#include "PageDirectory.h"
#include "VirtualAllocator.h"

namespace kernel {
    // defined in Core/crt0.asm
    extern "C" u8 kernel_page_directory[PageDirectory::directory_entry_count];

    PageDirectory* PageDirectory::s_kernel_dir;
    PageDirectory* PageDirectory::s_active_dir;

    PageDirectory::PageDirectory()
    {
    }

    PageDirectory::PageDirectory(RefPtr<Page> directory_page)
        : m_directory_page(directory_page),
          m_allocator(MemoryManager::userspace_usable_base,
                      MemoryManager::userspace_usable_length)
    {
        MemoryManager::the().inititalize(*this);
    }

    void PageDirectory::inititalize()
    {
        s_kernel_dir = new PageDirectory();

        auto as_physical
            = MemoryManager::kernel_address_as_physical(reinterpret_cast<ptr_t>(&kernel_page_directory));

        s_kernel_dir->m_directory_page = RefPtr<Page>::create(as_physical);

        s_kernel_dir->make_active();

        s_kernel_dir->allocator().set_range(MemoryManager::kernel_usable_base,
                                            MemoryManager::kernel_usable_length);

        // allocate a quickmap range
        auto range = s_kernel_dir->allocator().allocate_range(Page::size);
        MemoryManager::the().set_quickmap_range(range);
    }
    
    // TODO: Maybe add an overload of this function that takes in a const Page&
    void PageDirectory::map_page_directory_entry(size_t index, ptr_t physical_address)
    {
        ASSERT(is_active());
        ASSERT_PAGE_ALIGNED(physical_address);

        entry_at(index).set_physical_address(physical_address)
                       .make_supervisor_present();
    }

    void PageDirectory::store_physical_page(RefPtr<Page> page)
    {
        m_physical_pages.append(page);
    }

    void PageDirectory::map_page(ptr_t virtual_address, ptr_t physical_address)
    {
        ASSERT(is_active());
        ASSERT_PAGE_ALIGNED(virtual_address);
        ASSERT_PAGE_ALIGNED(physical_address);

        InterruptDisabler d;

        auto page_table_index = virtual_address / (4 * MB);
        auto page_entry = (virtual_address - (page_table_index * 4 * MB)) / (4 * KB);

        if (!entry_at(page_table_index).is_present())
        {
            log() << "PageDirectory: tried to access a non-present table " 
                  << page_table_index << ", allocating...";

            auto page = m_physical_pages.emplace(MemoryManager::the().allocate_page());
            map_page_directory_entry(page_table_index, page->address());
        }

        log() << "PageDirectory: mapping " << format::as_hex << virtual_address
              << " to " << physical_address << " at table:"  << format::as_dec
              << page_table_index << " entry: " << page_entry;

        table_at(page_table_index).entry_at(page_entry).set_physical_address(physical_address)
                                                       .make_supervisor_present();

        flush_at(virtual_address);
    }

    void PageDirectory::unmap_page(ptr_t virtual_address)
    {
        ASSERT(is_active());
        ASSERT_PAGE_ALIGNED(virtual_address);

        auto page_directory_entry = virtual_address / (4 * MB);
        auto page_entry = (virtual_address - (page_directory_entry * 4 * MB)) / 4 * KB;

        table_at(page_directory_entry).entry_at(page_entry).set_present(false);
        flush_at(virtual_address);
    }

    VirtualAllocator& PageDirectory::allocator()
    {
        return m_allocator;
    }

    PageDirectory& PageDirectory::of_kernel()
    {
        ASSERT(s_kernel_dir);

        return *s_kernel_dir;
    }

    bool PageDirectory::is_active()
    {
        ptr_t active_directory_ptr;

        asm("movl %%cr3, %%eax" : "=a"(active_directory_ptr));

        return physical_address() == active_directory_ptr;
    }

    void PageDirectory::make_active()
    {
        InterruptDisabler d;

        flush_all();
        s_active_dir = this;
    }

    void PageDirectory::flush_all()
    {
        asm volatile("movl %%eax, %%cr3" :: "a"(physical_address()) : "memory");
    }

    void PageDirectory::flush_at(ptr_t virtual_address)
    {
        log() << "PageDirectory: flushing a page at " << format::as_hex << virtual_address;

        asm volatile("invlpg %0" :: "m"(*reinterpret_cast<u8*>(virtual_address)) : "memory");
    }

    PageDirectory& PageDirectory::current()
    {
        ASSERT(s_active_dir);

        return *s_active_dir;
    }

    ptr_t PageDirectory::physical_address()
    {
        return m_directory_page->address();
    }

    PageTable& PageDirectory::table_at(size_t index)
    {
        ASSERT(is_active());

        return *reinterpret_cast<PageTable*>(recursive_table_base + table_size * index);
    }

    PageDirectory::Entry& PageDirectory::entry_at(size_t index)
    {
        ASSERT(is_active());

        return *reinterpret_cast<Entry*>(recursive_directory_base + directory_entry_size * index);
    }

    PageDirectory::Entry& PageDirectory::entry_at(size_t index, ptr_t virtual_base)
    {
        return *reinterpret_cast<PageDirectory::Entry*>(virtual_base + index * directory_entry_size);
    }
}
