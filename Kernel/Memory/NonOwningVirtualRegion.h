#pragma once

#include "Common/DynamicArray.h"
#include "Page.h"
#include "VirtualRegion.h"

namespace kernel {

class NonOwningVirtualRegion : public VirtualRegion {
public:
    NonOwningVirtualRegion(Range virtual_range, Range physical_range, Properties properties, StringView name)
        : VirtualRegion(virtual_range, properties, name)
        , m_physical_range(physical_range)
    {
    }

    const Range& physical_range() const { return m_physical_range; }

private:
    Range m_physical_range;
};

}