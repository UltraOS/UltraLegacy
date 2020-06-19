#pragma once

#include "Common/Types.h"

namespace kernel {
 
    class PageEntry
    {
    public:
        void set_physical_address(ptr_t address)
        {
            m_physical_address = address >> 12;
        }

        void set_attributes(u32 attributes)
        {
            m_attributes = attributes;
        }

        ptr_t physical_address() const
        {
            return m_physical_address << 12;
        }

        // TODO: turn into enum
        u32 attributes() const
        {
            return m_attributes;
        }
    private:
        u32   m_attributes       : 12;
        ptr_t m_physical_address : 20;
    };
}
