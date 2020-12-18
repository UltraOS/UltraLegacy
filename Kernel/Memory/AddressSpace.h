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
    static constexpr Address recursive_table_base = 0xFFC00000;
    static constexpr Address recursive_directory_base = 0xFFFFF000;
#endif

    using Entry = GenericPagingEntry;
    using Table = GenericPagingTable;

    class PT : public Table {
    };

    static_assert(sizeof(PT) == Page::size);

#ifdef ULTRA_64
    class PDT : public Table {
    public:
        PT& pt_at(size_t index) { return *accessible_address_of(index).as_pointer<PT>(); }
    };

    class PDPT : public Table {
    public:
        PDT& pdt_at(size_t index) { return *accessible_address_of(index).as_pointer<PDT>(); }
    };
#endif

    explicit AddressSpace(RefPtr<Page> directory_page);

    static void inititalize();
    static AddressSpace& of_kernel();
    static AddressSpace& current();

// clang-format off
#ifdef ULTRA_32
    PT&   pt_at(size_t index);
#elif defined(ULTRA_64)
    PDPT& pdpt_at(size_t index);
#endif
    // clang-format on

    Entry& entry_at(size_t index);

    VirtualAllocator& allocator();
    Address physical_address();

    void store_physical_page(RefPtr<Page> page);

    void map_page(Address virtual_address, Address physical_address, bool is_supervisor = true);

    void map_user_page(Address virtual_address, const Page& physical_address);
    void map_user_page(Address virtual_address, Address physical_address);

    void map_supervisor_page(Address virtual_address, const Page& physical_address);
    void map_supervisor_page(Address virtual_address, Address physical_address);

#ifdef ULTRA_64
    void map_huge_page(Address virtual_address, Address physical_address, bool is_supervior = true);

    void map_huge_user_page(Address virtual_address, const Page& physical_address);
    void map_huge_user_page(Address virtual_address, Address physical_address);

    void map_huge_supervisor_page(Address virtual_address, const Page& physical_address);
    void map_huge_supervisor_page(Address virtual_address, Address physical_address);
#endif

#ifdef ULTRA_32
    static Pair<size_t, size_t> virtual_address_as_paging_indices(Address virtual_address);
#elif defined(ULTRA_64)
    static Quad<size_t, size_t, size_t, size_t> virtual_address_as_paging_indices(Address virtual_address);
#endif

    void unmap_page(Address virtual_address);

    bool is_active();
    void make_active();
    void flush_all();
    void flush_at(Address virtual_address);

private:
    AddressSpace() = default;

    void map_supervisor_page_directory_entry(size_t index, Address physical_address);
    void map_supervisor_page_directory_entry(size_t index, const Page& physical_address);

    void map_user_page_directory_entry(size_t index, Address physical_address);
    void map_user_page_directory_entry(size_t index, const Page& physical_address);

    void map_page_directory_entry(size_t index, Address physical_address, bool is_supervior = true);

#ifdef ULTRA_32
    Entry& entry_at(size_t index, Address virtual_base);
#endif

private:
    DynamicArray<RefPtr<Page>> m_physical_pages;
    RefPtr<Page> m_main_page;
    VirtualAllocator m_allocator;

    RecursiveInterruptSafeSpinLock m_lock;

    static AddressSpace* s_of_kernel;
};
}
