#pragma once

#include "Common/Pair.h"
#include "Common/RefPtr.h"
#include "GenericPagingEntry.h"
#include "GenericPagingTable.h"
#include "Page.h"
#include "VirtualAllocator.h"

namespace kernel {

class AddressSpace {
public:
    friend class MemoryManager;

#ifdef ULTRA_32
    static constexpr Address recursive_table_base     = 0xFFC00000;
    static constexpr Address recursive_directory_base = 0xFFFFF000;
#endif

    using Entry = GenericPagingEntry;
    using Table = GenericPagingTable;

    class PT : public GenericPagingTable {
    };

    static_assert(sizeof(PT) == Page::size);

#ifdef ULTRA_64
    class PDT : public Table {
    public:
        PT& pt_at(size_t index) { return *accessible_address_of(index).as_pointer<PT>(); }
    };

    class PDPT : public Table {
        Entry& entry_at(size_t index) { return m_entries[index]; }

        PDT& pdt_at(size_t index) { return *accessible_address_of(index).as_pointer<PDT>(); }

    private:
        Entry m_entries[table_entry_count];
    };
#endif

    AddressSpace(RefPtr<Page> directory_page);

    static void          inititalize();
    static AddressSpace& of_kernel();
    static AddressSpace& current();

#ifdef ULTRA_32
    PT& pt_at(size_t index);
#elif defined(ULTRA_64)
    PDTP& pdpt_at(size_t index);
#endif

    Entry& entry_at(size_t index);

    VirtualAllocator& allocator();
    Address           physical_address();

    void store_physical_page(RefPtr<Page> page);

    void map_supervisor_page_directory_entry(size_t index, Address physical_address);
    void map_supervisor_page(Address virtual_address, Address physical_address);
    void map_supervisor_page_directory_entry(size_t index, const Page& physical_address);
    void map_supervisor_page(Address virtual_address, const Page& physical_address);
    void map_user_page_directory_entry(size_t index, Address physical_address);
    void map_user_page(Address virtual_address, Address physical_address);
    void map_user_page_directory_entry(size_t index, const Page& physical_address);
    void map_user_page(Address virtual_address, const Page& physical_address);

    void map_page_directory_entry(size_t index, Address physical_address, bool is_supervior = true);
    void map_page(Address virtual_address, Address physical_address, bool is_supervior = true);

    void unmap_page(Address virtual_address);

    bool is_active();
    void make_active();
    void flush_all();
    void flush_at(Address virtual_address);

private:
    AddressSpace();

#ifdef ULTRA_32
    Entry& entry_at(size_t index, Address virtual_base);
#endif

    Pair<size_t, size_t> virtual_address_as_paging_indices(Address virtual_address);

private:
    DynamicArray<RefPtr<Page>> m_physical_pages;
    RefPtr<Page>               m_main_page;
    VirtualAllocator           m_allocator;

    static AddressSpace* s_of_kernel;
    static AddressSpace* s_active;
};
}
