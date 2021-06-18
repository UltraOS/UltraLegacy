#include "PrivateVirtualRegion.h"
#include "MemoryManager.h"

namespace kernel {

PrivateVirtualRegion::PrivateVirtualRegion(Range range, Properties properties, StringView name)
    : VirtualRegion(range, properties, name)
{
}

void PrivateVirtualRegion::preallocate_entire(bool zeroed)
{
    m_owned_pages.expand_to(virtual_range().length() / Page::size);
    MemoryManager::the().preallocate(*this, zeroed);
}

void PrivateVirtualRegion::preallocate_specific(Range virtual_range, bool zeroed)
{
    auto offset_from_base = Page::round_down(virtual_range.end() - 1) - this->virtual_range().begin();
    m_owned_pages.expand_to(offset_from_base / Page::size + 1);

    MemoryManager::the().preallocate_specific(*this, virtual_range, zeroed);
}

Page PrivateVirtualRegion::page_at(Address virtual_address)
{
    auto offset_from_base = virtual_address - this->virtual_range().begin();
    offset_from_base /= Page::size;

    if (m_owned_pages.size() <= offset_from_base)
        return {};

    return m_owned_pages[offset_from_base];
}

void PrivateVirtualRegion::store_page(Page page, Address virtual_address)
{
    auto offset_from_base = virtual_address - this->virtual_range().begin();
    offset_from_base /= Page::size;

    m_owned_pages.expand_to(offset_from_base + 1);
    m_owned_pages[offset_from_base] = page;
}

}