#pragma once

#include "Common/Types.h"
#include "Common/DynamicBitArray.h"

namespace kernel {

    class PhysicalRegion
    {
    public:
        PhysicalRegion(ptr_t starting_address, size_t length);

        ptr_t  starting_address() const { return m_starting_address; }
        size_t free_pages()       const { return m_allocation_map.size(); }

        template<typename LoggerT>
        friend LoggerT& operator<<(LoggerT&& logger, const PhysicalRegion& region)
        {
            logger << "Starting address:" << format::as_hex
                   << region.m_starting_address  
                   << " page count: " << format::as_dec << region.free_pages();

            return logger;
        }
    private:
        ptr_t  m_starting_address { 0 };
        DynamicBitArray m_allocation_map;
    };
}
