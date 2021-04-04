#include "PrivateVirtualRegion.h"
#include "MemoryManager.h"

namespace kernel {

PrivateVirtualRegion::PrivateVirtualRegion(Range range, Properties properties, StringView name)
    : VirtualRegion(range, properties, name)
{
}

void PrivateVirtualRegion::preallocate_entire(bool zeroed)
{
    MemoryManager::the().preallocate(*this, zeroed);
}

void PrivateVirtualRegion::preallocate_specific(Range virtual_range, bool zeroed)
{
    MemoryManager::the().preallocate_specific(*this, virtual_range, zeroed);
}

void PrivateVirtualRegion::store_page(Page page)
{
    m_owned_pages.append(page);
}

}