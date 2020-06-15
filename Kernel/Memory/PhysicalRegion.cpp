#include "MemoryManager.h"
#include "PhysicalRegion.h"
#include "Page.h"

#define PHYSICAL_REGION_DEBUG

namespace kernel {

    PhysicalRegion::PhysicalRegion(ptr_t starting_address, size_t length)
        : m_starting_address(starting_address)
    {
        m_allocation_map.set_size(length / Page::size);
    }

    ptr_t PhysicalRegion::bit_as_physical_address(size_t bit)
    {
        return m_starting_address + bit * Page::size;
    }

    size_t PhysicalRegion::physical_address_as_bit(ptr_t address)
    {
        ASSERT((address % Page::size) == 0);

        return (address - m_starting_address) / Page::size;
    }

    Page PhysicalRegion::allocate_page()
    {
        auto index = m_allocation_map.find_range(1, false);

        if (index == -1)
        {
            error() << "PhysicalRegion: Failed to allocate a physical page (Out of pages)!";
            hang();
        }

        m_allocation_map.set_bit(index, true);

        #ifdef PHYSICAL_REGION_DEBUG
        log() << "PhysicalRegion: allocating a page at address "
              << format::as_hex << bit_as_physical_address(index);
        #endif

        return { bit_as_physical_address(index) };
    }

    DynamicArray<Page> PhysicalRegion::allocate_pages(size_t count)
    {
        auto index = m_allocation_map.find_range(count, false);

        if (index == -1)
        {
            error() << "PhysicalRegion: Failed to allocate physical pages (Out of pages)!";
            hang();
        }

        DynamicArray<Page> pages(count);

        for (size_t i = 0; i < count; ++i)
        {
            m_allocation_map.set_bit(index + i, true);

            #ifdef PHYSICAL_REGION_DEBUG
            log() << "PhysicalRegion: allocating a page at address"
                  << format::as_hex << bit_as_physical_address(index + i);
            #endif

            pages.emplace(bit_as_physical_address(index + i));
        }

        return pages;
    }

    void PhysicalRegion::return_page(const Page& page)
    {
        #ifdef PHYSICAL_REGION_DEBUG
        log() << "PhysicalRegion: deallocating a page at index " 
              << physical_address_as_bit(page.address())
              << " Address:" << format::as_hex << page.address();
        #endif

        ASSERT(page.address() >= m_starting_address &&
               page.address() < bit_as_physical_address(m_allocation_map.size() - 1));

        auto bit = physical_address_as_bit(page.address());

        m_allocation_map.set_bit(bit, false);
    }

    void PhysicalRegion::return_pages(const DynamicArray<Page>& pages)
    {
        for (const auto& page : pages)
            return_page(page);
    }
}
