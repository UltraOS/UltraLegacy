#pragma once
#include "Types.h"
#include "Memory/HeapAllocator.h"

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

inline void* operator new(size_t size)
{
    return kernel::HeapAllocator::allocate(size);
}

inline void* operator new[](size_t size)
{
    return kernel::HeapAllocator::allocate(size);
}

inline void operator delete(void* ptr) noexcept
{
    kernel::HeapAllocator::free(ptr);
}

inline void operator delete[](void* ptr) noexcept
{
    kernel::HeapAllocator::free(ptr);
}

inline void operator delete(void* ptr, size_t) noexcept
{
    kernel::HeapAllocator::free(ptr);
}

inline void operator delete[](void* ptr, size_t) noexcept
{
    kernel::HeapAllocator::free(ptr);
}
