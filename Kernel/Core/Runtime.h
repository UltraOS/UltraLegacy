#pragma once
#include "Core/Types.h"
#include "Memory/Allocator.h"

namespace kernel::runtime {
    void ensure_loaded_correctly();
    void init_global_objects();
}

inline void* operator new(size_t, void* ptr)
{
    return ptr;
}

inline void* operator new[](size_t, void* ptr)
{
    return ptr;
}

inline void operator delete(void*, void*) noexcept
{
}

inline void operator delete[](void*, void*) noexcept
{
}

inline void operator delete(void*, size_t) noexcept
{
}

inline void operator delete[](void*, size_t) noexcept
{
}

inline void *operator new(size_t size)
{
    return kernel::Allocator::allocate(size);
}

inline void *operator new[](size_t size)
{
    return kernel::Allocator::allocate(size);
}

inline void operator delete(void* ptr) noexcept
{
    kernel::Allocator::free(ptr);
}

inline void operator delete[](void* ptr) noexcept
{
    kernel::Allocator::free(ptr);
}
