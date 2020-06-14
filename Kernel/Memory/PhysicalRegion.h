#pragma once

#include "Common/Types.h"

namespace kernel {

    class PhysicalRegion
    {
    public:
        PhysicalRegion(ptr_t starting_address);

        void expand();
        void seal();

        template<typename LoggerT>
        friend LoggerT& operator<<(LoggerT&& logger, const PhysicalRegion& region)
        {
            logger << "Starting address:" << format::as_hex
                   << region.m_starting_address  
                   << " page count: " << format::as_dec << region.m_page_count;

            return logger;
        }
    private:
        ptr_t  m_starting_address { 0 };
        size_t m_page_count       { 0 };
        // Bitmap m_page_map;
    };
}
