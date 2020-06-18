#include "PageTable.h"
#include "PageDirectory.h"

namespace kernel {
    // defined in Core/crt0.asm
    extern "C" u8 kernel_page_directory[PageDirectory::directory_entry_count];

    PageDirectory* PageDirectory::s_kernel_dir;

    PageDirectory::PageDirectory()
    {
    }

    PageDirectory::PageDirectory(RefPtr<Page> directory_page)
        : m_directory_page(directory_page),
          m_allocator(MemoryManager::userspace_base,
                      MemoryManager::userspace_length)
    {
        MemoryManager::the().inititalize(*this);
    }

    void PageDirectory::inititalize()
    {
        s_kernel_dir = new PageDirectory();

        auto as_physical = reinterpret_cast<ptr_t>(&kernel_page_directory) - MemoryManager::kernel_base;

        s_kernel_dir->m_directory_page =
            RefPtr<Page>::create(as_physical);

        s_kernel_dir->make_active();

        s_kernel_dir->allocator().set_range(MemoryManager::kernel_base, MemoryManager::kernel_length);
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

        return m_directory_page->address() == active_directory_ptr;
    }

    void PageDirectory::make_active()
    {
        asm volatile("movl %%eax, %%cr3" :: "a"(m_directory_page->address()) : "memory");
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
