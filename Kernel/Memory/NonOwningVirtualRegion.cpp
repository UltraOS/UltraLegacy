#include "NonOwningVirtualRegion.h"
#include "AddressSpace.h"

namespace kernel {

void NonOwningVirtualRegion::switch_physical_range_and_remap(Range physical_range)
{
    ASSERT(physical_range.length() == m_physical_range.length());

    AddressSpace::current().map_range(virtual_range(), physical_range, is_supervisor());
}

}
