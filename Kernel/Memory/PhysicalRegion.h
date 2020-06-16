#pragma once

#include "Common/Types.h"
#include "Common/DynamicBitArray.h"
#include "Common/RefPtr.h"

namespace kernel {

    class Page;

    class PhysicalRegion
    {
    public:
        PhysicalRegion(ptr_t starting_address, size_t length);

        ptr_t  starting_address() const { return m_starting_address; }
        size_t free_page_count()  const { return m_free_pages;       }
        bool   has_free_pages()   const { return m_free_pages;       }

        [[nodiscard]] RefPtr<Page> allocate_page();
        void free_page(const Page& page);

        bool contains(const Page& page);

        template<typename LoggerT>
        friend LoggerT& operator<<(LoggerT&& logger, const PhysicalRegion& region)
        {
            logger << "Starting address:" << format::as_hex
                   << region.m_starting_address  
                   << " page count: " << format::as_dec << region.free_page_count();

            return logger;
        }
    private:
        ptr_t bit_as_physical_address(size_t bit);
        size_t physical_address_as_bit(ptr_t address);
    private:
        ptr_t  m_starting_address { 0 };
        size_t m_free_pages       { 0 };
        DynamicBitArray m_allocation_map;
    };
}
