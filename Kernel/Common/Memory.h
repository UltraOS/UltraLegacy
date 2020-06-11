#pragma once

#include "Types.h"

namespace kernel {

    inline void set_memory(void* ptr, size_t size, u8 value)
    {
        auto* byte_ptr = reinterpret_cast<u8*>(ptr);

        for (size_t i = 0; i < size; ++i)
        {
            byte_ptr[i] = value;
        }
    }

    inline void zero_memory(void* ptr, size_t size)
    {
        set_memory(ptr, size, 0);
    }
}
