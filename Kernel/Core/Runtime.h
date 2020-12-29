#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"
#include "Memory/HeapAllocator.h"
#include "Registers.h"

namespace kernel::runtime {

void ensure_loaded_correctly();
void init_global_objects();

class KernelSymbolTable {
public:
    static constexpr size_t max_symbols = 1024;
    static constexpr Address physical_symbols_base = 0x45000;

    class Symbol {
    public:
        Symbol() = default;

        Symbol(ptr_t address, char* name)
            : m_address(address)
            , m_name(name)
        {
        }

        ptr_t address() const { return m_address; }
        const char* name() const { return m_name; }

        friend bool operator<(ptr_t address, const Symbol& symbol) { return address < symbol.address(); }

    private:
        ptr_t m_address { 0 };
        char* m_name { nullptr };
    };

    static bool symbols_available() { return s_symbol_count > 0; }
    static void parse_all();
    static bool is_address_within_symbol_table(ptr_t address);
    static const Symbol* find_symbol(ptr_t address);

private:
    static ptr_t s_symbols_begin;
    static ptr_t s_symbols_end;

    static size_t s_symbol_count;
    static Symbol s_symbols[max_symbols];
};

ALWAYS_INLINE inline Address base_pointer();
inline Address base_pointer()
{
    Address base_pointer;

#ifdef ULTRA_32
    asm("mov %%ebp, %0"
        : "=a"(base_pointer));
#elif defined(ULTRA_64)
    asm("mov %%rbp, %0"
        : "=a"(base_pointer));
#endif

    return base_pointer;
}

size_t dump_backtrace(ptr_t* into, size_t max_depth, Address base_pointer);

ALWAYS_INLINE inline size_t dump_backtrace(ptr_t* into, size_t max_depth);
inline size_t dump_backtrace(ptr_t* into, size_t max_depth)
{
    return dump_backtrace(into, max_depth, base_pointer());
}

[[noreturn]] void on_assertion_failed(const char* message, const char* file, const char* function, u32 line);
[[noreturn]] void panic(const char* reason, const RegisterState* = nullptr);

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

#define ASSERT(expression)         \
    (static_cast<bool>(expression) \
            ? static_cast<void>(0) \
            : kernel::runtime::on_assertion_failed(TO_STRING(expression), __FILE__, __PRETTY_FUNCTION__, __LINE__))

#define ASSERT_NEVER_REACHED() ASSERT(!"Tried to execute a non-reachable area")
