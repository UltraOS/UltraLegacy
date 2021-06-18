#include "SharedVirtualRegion.h"
#include "MemoryManager.h"

namespace kernel {

SharedVirtualRegion::SharedVirtualRegion(Range range, Properties properties, StringView name)
    : VirtualRegion(range, properties, name)
{
    m_shared_block = new SharedBlock;
    m_shared_block->ref_count = 1;
}

SharedVirtualRegion::SharedVirtualRegion(Range range, Properties properties, const SharedVirtualRegion& to_clone)
    : VirtualRegion(range, properties, to_clone.name().to_view())
    , m_shared_block(to_clone.m_shared_block)
{
    m_shared_block->ref_count++;
}

SharedVirtualRegion* SharedVirtualRegion::clone(Range virtual_range, IsSupervisor is_supervisor)
{
    Properties props {};
    if (is_supervisor == IsSupervisor::YES)
        props += Properties::SUPERVISOR;
    props += Properties::SHARED;

    return new SharedVirtualRegion(virtual_range, props, *this);
}

void SharedVirtualRegion::preallocate_entire(bool zeroed)
{
    m_shared_block->pages.expand_to(virtual_range().length() / Page::size);
    MemoryManager::the().preallocate(*this, zeroed);
}

void SharedVirtualRegion::preallocate_specific(Range virtual_range, bool zeroed)
{
    auto offset_from_base = Page::round_down(virtual_range.end() - 1) - this->virtual_range().begin();
    m_shared_block->pages.expand_to(offset_from_base / Page::size + 1);

    MemoryManager::the().preallocate_specific(*this, virtual_range, zeroed);
}

void SharedVirtualRegion::store_page(Page page, Address virtual_address)
{
    auto offset_from_base = virtual_address - this->virtual_range().begin();
    offset_from_base /= Page::size;

    m_shared_block->pages.expand_to(offset_from_base + 1);
    m_shared_block->pages[offset_from_base] = page;
}

Page SharedVirtualRegion::page_at(Address virtual_address)
{
    auto offset_from_base = virtual_address - this->virtual_range().begin();
    offset_from_base /= Page::size;

    if (m_shared_block->pages.size() <= offset_from_base)
        return {};

    return m_shared_block->pages[offset_from_base];
}

}