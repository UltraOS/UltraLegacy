#pragma once

#include "Common/DynamicArray.h"
#include "Common/Pair.h"
#include "Common/RefPtr.h"
#include "GenericPagingEntry.h"
#include "GenericPagingTable.h"
#include "Page.h"
#include "VirtualAllocator.h"
#include "VirtualRegion.h"

namespace kernel {

class AddressSpace {
public:
    friend class MemoryManager;

#ifdef ULTRA_32
    static constexpr Address recursive_table_base = 0xFFC00000;
    static constexpr Address recursive_directory_base = 0xFFFFF000;
#elif defined(ULTRA_64)
    static constexpr size_t lower_identity_size = 4 * GB;
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

    AddressSpace();

    static void inititalize();
    static bool is_initialized()
    {
        return s_of_kernel != nullptr;
    }

    static AddressSpace& of_kernel();
    static AddressSpace& current();

#ifdef ULTRA_32
    PT& pt_at(size_t index);
    static PT& kernel_pt_at(size_t index);
#elif defined(ULTRA_64)
    PDPT& pdpt_at(size_t index);
    static PDPT& kernel_pdpt_at(size_t index);
#endif

    Entry& entry_at(size_t index);
    static Entry& kernel_entry_at(size_t index);

    VirtualAllocator& allocator();
    Address physical_address() const;

    // Not thread safe, the directory is assumed to be inactive
    DynamicArray<Page>& owned_pages() { return m_physical_pages; }

    void map_page(Address virtual_address, Address physical_address, IsSupervisor = IsSupervisor::YES);
    void map_range(Range virtual_range, Range physical_range, IsSupervisor = IsSupervisor::YES);

    // Maps a page into the kernel address space
    // Expects all paging structures to be present and valid
    static void early_map_page(Address virtual_address, Address physical_address);
    static void early_map_range(Range virtual_range, Range physical_range);
#ifdef ULTRA_64
    static void early_map_huge_page(Address virtual_address, Address physical_address);
    static void early_map_huge_range(Range virtual_range, Range physical_range);
#endif

#ifdef ULTRA_64
    void map_huge_page(Address virtual_address, Address physical_address, IsSupervisor = IsSupervisor::YES);
    void map_huge_range(Range virtual_range, Range physical_range, IsSupervisor = IsSupervisor::YES);
#endif

#ifdef ULTRA_32
    static Pair<size_t, size_t> virtual_address_as_paging_indices(Address virtual_address);
#elif defined(ULTRA_64)
    static Quad<size_t, size_t, size_t, size_t> virtual_address_as_paging_indices(Address virtual_address);
#endif

    void local_unmap_page(Address virtual_address);
    void local_unmap_range(const Range&);
    void unmap_page(Address virtual_address);
    void unmap_range(const Range&);

    Address physical_address_of(Address);

    bool is_active() const;
    void make_active();
    void invalidate_all();
    void invalidate_range(Range virtual_range);
    void invalidate_at(Address virtual_address);

    bool is_of_kernel() const;

private:
    void map_page_directory_entry(size_t index, Address physical_address, IsSupervisor);

#ifdef ULTRA_32
    Entry& entry_at(size_t index, Address virtual_base);
#endif

    static Address active_directory_address();

    friend bool operator==(const AddressSpace& l, const AddressSpace& r)
    {
        return l.m_main_page.address() == r.m_main_page.address();
    }

    friend bool operator!=(const AddressSpace& l, const AddressSpace& r)
    {
        return !operator==(l, r);
    }

private:
    DynamicArray<Page> m_physical_pages;
    Page m_main_page;
    VirtualAllocator m_allocator;

    RecursiveInterruptSafeSpinLock m_lock;

    static AddressSpace* s_of_kernel;
};
}
