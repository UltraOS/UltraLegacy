#pragma once

#include "Common/DynamicArray.h"
#include "Page.h"
#include "VirtualRegion.h"

namespace kernel {

class PrivateVirtualRegion : public VirtualRegion {
public:
    PrivateVirtualRegion(Range range, Properties properties, StringView name);

    void preallocate_entire(bool zeroed = true);
    void preallocate_specific(Range, bool zeroed = true);

    InterruptSafeSpinLock& lock() { return m_lock; }

private:
    friend class MemoryManager;
    Page page_at(Address virtual_address);
    void store_page(Page page, Address virtual_address);
    DynamicArray<Page>& owned_pages() { return m_owned_pages; }

private:
    InterruptSafeSpinLock m_lock;
    DynamicArray<Page> m_owned_pages;
};

}