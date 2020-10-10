#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"
#include "Memory/HeapAllocator.h"

namespace kernel::runtime {
void              ensure_loaded_correctly();
void              init_global_objects();
void              parse_kernel_symbols();
[[noreturn]] void on_assertion_failed(const char* message, const char* file, const char* function, u32 line);
[[noreturn]] void panic(const char* reason);
[[noreturn]] void panic(const char* reason, Address base_pointer, Address instruction_pointer = nullptr);
}

inline void* operator new(size_t, void* ptr)
{
    return ptr;
}

inline void* operator new[](size_t, void* ptr)
{
    return ptr;
}

inline void operator delete(void*, void*) noexcept { }

inline void operator delete[](void*, void*) noexcept { }

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

#define ASSERT(expression)                                                                                             \
    (static_cast<bool>(expression)                                                                                     \
         ? static_cast<void>(0)                                                                                        \
         : kernel::runtime::on_assertion_failed(TO_STRING(expression), __FILE__, __PRETTY_FUNCTION__, __LINE__))

#define ASSERT_NEVER_REACHED() ASSERT(!"Tried to execute a non-reachable area")
