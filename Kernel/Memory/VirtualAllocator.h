#pragma once

#include "Common/Types.h"

namespace kernel {

    class VirtualAllocator
    {
    public:
        VirtualAllocator();
        VirtualAllocator(ptr_t base, size_t length);

        void set_range(ptr_t base, size_t length);

    private:
        ptr_t  m_base   { 0 };
        size_t m_length { 0 };
    };
}
