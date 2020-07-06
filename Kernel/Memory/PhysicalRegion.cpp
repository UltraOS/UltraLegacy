#include "PhysicalRegion.h"
#include "MemoryManager.h"
#include "Page.h"

#define PHYSICAL_REGION_DEBUG

namespace kernel {

PhysicalRegion::PhysicalRegion(Address starting_address, size_t length)
    : m_starting_address(starting_address), m_free_pages(length / Page::size), m_allocation_map(m_free_pages)
{
}

Address PhysicalRegion::bit_as_physical_address(size_t bit)
{
    return m_starting_address + bit * Page::size;
}

size_t PhysicalRegion::physical_address_as_bit(Address address)
{
    ASSERT_PAGE_ALIGNED(address);

    return (address - m_starting_address) / Page::size;
}

RefPtr<Page> PhysicalRegion::allocate_page()
{
    auto index = m_allocation_map.find_range(1, false);

    if (index == -1) {
        warning() << "PhysicalRegion: Failed to allocate a physical page (Out of pages)!";
        return {};
    }

    m_allocation_map.set_bit(index, true);
    --m_free_pages;

#ifdef PHYSICAL_REGION_DEBUG
    log() << "PhysicalRegion: allocating a page at address " << bit_as_physical_address(index);
#endif

    return RefPtr<Page>::create(bit_as_physical_address(index));
}

bool PhysicalRegion::contains(const Page& page)
{
    return page.address() >= m_starting_address
           && page.address() < bit_as_physical_address(m_allocation_map.size() - 1);
}

void PhysicalRegion::free_page(const Page& page)
{
#ifdef PHYSICAL_REGION_DEBUG
    log() << "PhysicalRegion: deallocating a page at index " << physical_address_as_bit(page.address())
          << " Address:" << page.address();
#endif

    ASSERT(contains(page));

    auto bit = physical_address_as_bit(page.address());

    m_allocation_map.set_bit(bit, false);
    ++m_free_pages;
}
}
