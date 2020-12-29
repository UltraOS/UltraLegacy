#include "PhysicalRegion.h"
#include "MemoryManager.h"
#include "Page.h"

// #define PHYSICAL_REGION_DEBUG

namespace kernel {

PhysicalRegion::PhysicalRegion(const Range& range)
    : m_range(range)
    , m_free_pages(range.length() / Page::size)
    , m_allocation_map(m_free_pages)
{
    ASSERT_PAGE_ALIGNED(Address(range.begin()));
    ASSERT_PAGE_ALIGNED(range.length());
}

Address PhysicalRegion::bit_as_physical_address(size_t bit)
{
    return m_range.begin() + bit * Page::size;
}

size_t PhysicalRegion::physical_address_as_bit(Address address)
{
    ASSERT_PAGE_ALIGNED(address);

    return (address - m_range.begin()) / Page::size;
}

Page PhysicalRegion::allocate_page()
{
    auto index = m_allocation_map.find_bit(false, m_next_hint);

    if (index == -1) {
        StackStringBuilder error_string;
        error_string << "PhysicalRegion: Failed to allocate a physical page (Out of pages)!";
        runtime::panic(error_string.data());
    }

    m_next_hint = static_cast<size_t>(index + 1);

    // reset hint in case we got the last index
    if (m_next_hint == m_allocation_map.size())
        m_next_hint = 0;

    m_allocation_map.set_bit(index, true);
    --m_free_pages;

#ifdef PHYSICAL_REGION_DEBUG
    log() << "PhysicalRegion: allocating a page at address " << bit_as_physical_address(index);
#endif

    return Page(bit_as_physical_address(index));
}

void PhysicalRegion::free_page(const Page& page)
{
#ifdef PHYSICAL_REGION_DEBUG
    log() << "PhysicalRegion: deallocating a page at index " << physical_address_as_bit(page.address())
          << " Address:" << page.address();
#endif

    ASSERT(m_range.contains(page.address()));

    auto bit = physical_address_as_bit(page.address());

    m_allocation_map.set_bit(bit, false);
    ++m_free_pages;
}
}
