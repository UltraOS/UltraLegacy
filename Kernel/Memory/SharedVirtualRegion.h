#pragma once

#include "Common/DynamicArray.h"
#include "Page.h"
#include "VirtualRegion.h"

namespace kernel {

class SharedVirtualRegion : public VirtualRegion {
public:
    SharedVirtualRegion(Range range, Properties properties, StringView name);

    void preallocate_entire(bool zeroed = true);
    void preallocate_specific(Range, bool zeroed = true);

    ~SharedVirtualRegion();

private:
    SharedVirtualRegion(Range range, Properties properties, const SharedVirtualRegion& to_clone);
    SharedVirtualRegion* clone(Range virtual_range, IsSupervisor);

    struct SharedBlock {
        InterruptSafeSpinLock modification_lock;
        DynamicArray<Page> pages;
        Atomic<size_t> ref_count;
    } * m_shared_block;

    friend class MemoryManager;

    InterruptSafeSpinLock& lock() { return m_shared_block->modification_lock; }
    size_t decref() { return --m_shared_block->ref_count; }
    SharedBlock& shared_block() { return *m_shared_block; }

    void store_page(Page page, Address virtual_address);
    Page page_at(Address virtual_address);
    DynamicArray<Page>& owned_pages() { return m_shared_block->pages; }
};

}