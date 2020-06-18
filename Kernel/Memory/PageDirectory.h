#pragma once

#include "Common/RefPtr.h"
#include "Page.h"
#include "PageTable.h"
#include "VirtualAllocator.h"

namespace kernel {

    class PageDirectory
    {
    public:
        friend class MemoryManager;

        struct Entry
        {
            u32   properties : 12;
            ptr_t table_ptr : 20;
        };

        static constexpr ptr_t  recursive_directory_base = 0xFFFFF000;
        static constexpr size_t directory_entry_size = sizeof(ptr_t);
        static constexpr ptr_t  recursive_table_base = 0xFFC00000;
        static constexpr size_t table_entry_count = 1024;
        static constexpr size_t directory_entry_count = 1024;
        static constexpr size_t table_entry_size = sizeof(ptr_t);
        static constexpr size_t table_size = table_entry_count * table_entry_size;

        PageDirectory(RefPtr<Page> directory_page);

        static void inititalize();
        static PageDirectory& of_kernel();

        PageTable& table_at(size_t index);
        Entry& entry_at(size_t index);
        VirtualAllocator& allocator();

        bool is_active();
        void make_active();
    private:
        PageDirectory();

        Entry& entry_at(size_t index, ptr_t virtual_base);

    private:
        DynamicArray<RefPtr<Page>> m_physical_pages;
        RefPtr<Page>               m_directory_page;
        VirtualAllocator           m_allocator;

        static PageDirectory* s_kernel_dir;
    };
}
