#pragma once

#include "Common/Pair.h"
#include "Common/RefPtr.h"
#include "Page.h"
#include "PageEntry.h"
#include "PageTable.h"
#include "VirtualAllocator.h"

namespace kernel {

class PageDirectory {
public:
    friend class MemoryManager;

    using Entry = PageEntry;

    static constexpr ptr_t  recursive_directory_base = 0xFFFFF000;
    static constexpr size_t directory_entry_size     = sizeof(ptr_t);
    static constexpr ptr_t  recursive_table_base     = 0xFFC00000;
    static constexpr size_t table_entry_count        = 1024;
    static constexpr size_t directory_entry_count    = 1024;
    static constexpr size_t table_entry_size         = sizeof(ptr_t);
    static constexpr size_t table_size               = table_entry_count * table_entry_size;

    PageDirectory(RefPtr<Page> directory_page);

    static void           inititalize();
    static PageDirectory& of_kernel();
    static PageDirectory& current();

    PageTable&        table_at(size_t index);
    Entry&            entry_at(size_t index);
    VirtualAllocator& allocator();
    ptr_t             physical_address();

    void store_physical_page(RefPtr<Page> page);

    void map_supervisor_page_directory_entry(size_t index, ptr_t physical_address);
    void map_supervisor_page(ptr_t virtual_address, ptr_t physical_address);
    void map_supervisor_page_directory_entry(size_t index, const Page& physical_address);
    void map_supervisor_page(ptr_t virtual_address, const Page& physical_address);
    void map_user_page_directory_entry(size_t index, ptr_t physical_address);
    void map_user_page(ptr_t virtual_address, ptr_t physical_address);
    void map_user_page_directory_entry(size_t index, const Page& physical_address);
    void map_user_page(ptr_t virtual_address, const Page& physical_address);

    void map_page_directory_entry(size_t index, ptr_t physical_address, bool is_supervior = true);
    void map_page(ptr_t virtual_address, ptr_t physical_address, bool is_supervior = true);

    void unmap_page(ptr_t virtual_address);

    bool is_active();
    void make_active();
    void flush_all();
    void flush_at(ptr_t virtual_address);

private:
    PageDirectory();

    Entry& entry_at(size_t index, ptr_t virtual_base);

    Pair<size_t, size_t> virtual_address_as_paging_indices(ptr_t virtual_address);

private:
    DynamicArray<RefPtr<Page>> m_physical_pages;
    RefPtr<Page>               m_directory_page;
    VirtualAllocator           m_allocator;

    static PageDirectory* s_kernel_dir;
    static PageDirectory* s_active_dir;
};
}
