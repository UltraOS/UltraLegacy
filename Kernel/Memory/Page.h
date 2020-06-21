#pragma once

#include "Common/Types.h"
#include "Core/Runtime.h"

namespace kernel {

    class Page
    {
    public:
        static constexpr size_t size           = 4096;
        static constexpr ptr_t  alignment_mask = 0xFFFFF000;

        Page(ptr_t physical_address);

        ptr_t address() const;

        static constexpr bool is_aligned(ptr_t address)
        {
            return (address % size) == 0;
        }

        ~Page();

    private:
        ptr_t m_physical_address;
    };

    #define ASSERT_PAGE_ALIGNED(address) ASSERT(Page::is_aligned(address));
}
