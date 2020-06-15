#pragma once

#include "Common/Types.h"
#include "Common/DynamicBitArray.h"
#include "Page.h"

namespace kernel {

    class PhysicalRegion
    {
    public:
        PhysicalRegion(ptr_t starting_address, size_t length);

        ptr_t  starting_address() const { return m_starting_address; }
        size_t free_pages()       const { return m_allocation_map.size(); }

                     Page  allocate_page();
        DynamicArray<Page> allocate_pages(size_t count);

        void return_page(const Page& page);
        void return_pages(const DynamicArray<Page>& pages);

        template<typename LoggerT>
        friend LoggerT& operator<<(LoggerT&& logger, const PhysicalRegion& region)
        {
            logger << "Starting address:" << format::as_hex
                   << region.m_starting_address  
                   << " page count: " << format::as_dec << region.free_pages();

            return logger;
        }
    private:
        ptr_t bit_as_physical_address(size_t bit);
        size_t physical_address_as_bit(ptr_t address);
    private:
        ptr_t m_starting_address { 0 };
        DynamicBitArray m_allocation_map;
    };
}
