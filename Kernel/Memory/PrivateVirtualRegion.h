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

private:
    friend class MemoryManager;
    void store_page(Page page);
    DynamicArray<Page>& owned_pages() { return m_owned_pages; }

private:
    DynamicArray<Page> m_owned_pages;
};

}