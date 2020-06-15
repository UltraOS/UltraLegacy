#pragma once

#include "Common/Types.h"

namespace kernel {

    class Page
    {
    public:
        static constexpr size_t size = 4096;

        Page(ptr_t physical_address)
            : m_physical_address(physical_address)
        {
        }

        ptr_t address() const
        {
            return m_physical_address;
        }

    private:
        ptr_t m_physical_address;
    };
}
