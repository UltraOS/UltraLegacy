#pragma once

#include "Types.h"

namespace kernel {

inline void set_memory(void* ptr, size_t size, u8 value)
{
    auto* byte_ptr = reinterpret_cast<u8*>(ptr);

    for (size_t i = 0; i < size; ++i) {
        byte_ptr[i] = value;
    }
}

inline void zero_memory(void* ptr, size_t size)
{
    set_memory(ptr, size, 0);
}

inline void copy_memory(const void* source, void* destination, size_t size)
{
    const u8* byte_src = reinterpret_cast<const u8*>(source);
    u8* byte_dst = reinterpret_cast<u8*>(destination);

    while (size--)
        *byte_dst++ = *byte_src++;
}

inline void move_memory(const void* source, void* destination, size_t size)
{
    const u8* byte_src = reinterpret_cast<const u8*>(source);
    u8* byte_dst = reinterpret_cast<u8*>(destination);

    if (source < destination) {
        byte_src += size;
        byte_dst += size;

        while (size--)
            *--byte_dst = *--byte_src;
    } else
        copy_memory(source, destination, size);
}

inline bool compare_memory(const void* lhs, const void* rhs, size_t size)
{
    const u8* byte_lhs = reinterpret_cast<const u8*>(lhs);
    const u8* byte_rhs = reinterpret_cast<const u8*>(rhs);

    while (size--) {
        if (*byte_lhs++ != *byte_rhs++)
            return false;
    }

    return true;
}

}
