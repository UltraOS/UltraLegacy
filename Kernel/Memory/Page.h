#pragma once

#include "Common/Types.h"
#include "PhysicalRegion.h"
#include "MemoryManager.h"

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

        ~Page()
        {
            MemoryManager::the().free_page(*this);
        }

    private:
        ptr_t m_physical_address;
    };
}
