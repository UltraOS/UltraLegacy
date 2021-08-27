#pragma once

#include "Core/Runtime.h"

namespace kernel {

template <typename T>
inline void set_dwords_to_address(T& low, T& hi, Address address)
{
    static_assert(sizeof(T) == 4);
#ifdef ULTRA_64
    static constexpr u32 dword_mask = 0xFFFFFFFF;
    low = address & dword_mask;
    hi = (address >> 32) & dword_mask;
#elif defined(ULTRA_32)
    low = address;
    hi = 0;
#endif
}

template <typename T>
inline Address address_from_dwords(T& low, T& hi)
{
    static_assert(sizeof(T) == 4);

    ptr_t out = 0;
#ifdef ULTRA_32
    ASSERT(hi == 0);
    out = low;
#elif defined(ULTRA_64)
    out = low | (static_cast<u64>(hi) << 32);
#endif

    return out;
}

template <typename T>
T mmio_read32(Address address)
{
    static_assert(sizeof(T) == 4);

    auto value = *address.as_pointer<volatile u32>();
    return bit_cast<T>(value);
}

template <typename T>
void mmio_write32(Address address, T value)
{
    static_assert(sizeof(T) == 4);

    auto raw = bit_cast<u32>(value);
    *address.as_pointer<volatile u32>() = raw;
}

enum class MMIOPolicy {
    // ignored for 32 bit builds, DWORD_ACCESS is used instead
    QWORD_ACCESS,

    // if T is a QWORD value, it is combined by reading lower, then upper halves
    DWORD_ACCESS,

    // reads the lower half of a QWORD from address,
    // upper half is zeroed for reads, or ignored for writes
    IGNORE_UPPER_DWORD,
};

template <typename T>
T mmio_read64(Address address, MMIOPolicy policy)
{
    switch (policy) {
    case MMIOPolicy::QWORD_ACCESS: {
#ifdef ULTRA_64
        auto value = *address.as_pointer<volatile u64>();
        return bit_cast<T>(value);
#elif defined(ULTRA_32)
        [[fallthrough]];
#endif
    }
    case MMIOPolicy::DWORD_ACCESS: {
        u32 reg_as_u32[2];
        reg_as_u32[0] = *address.as_pointer<volatile u32>();
        address += sizeof(u32);
        reg_as_u32[1] = *address.as_pointer<volatile u32>();
        return bit_cast<T>(reg_as_u32);
    }
    case MMIOPolicy::IGNORE_UPPER_DWORD: {
        u32 reg_as_u32[2] {};
        reg_as_u32[0] = *address.as_pointer<volatile u32>();
        return bit_cast<T>(reg_as_u32);
    }
    default:
        ASSERT_NEVER_REACHED();
    }
}

template <typename T>
void mmio_write64(Address address, T value, MMIOPolicy policy)
{
    static_assert(sizeof(T) == 8);

    switch (policy) {
    case MMIOPolicy::QWORD_ACCESS: {
#ifdef ULTRA_64
        volatile auto raw = bit_cast<u64>(value);
        *address.as_pointer<volatile u64>() = raw;
        return;
#elif defined(ULTRA_32)
        [[fallthrough]];
#endif
    }
    case MMIOPolicy::DWORD_ACCESS: {
        u32 reg_as_u32[2];
        copy_memory(&value, reg_as_u32, 2 * sizeof(u32));
        *address.as_pointer<volatile u32>() = reg_as_u32[0];
        address += sizeof(u32);
        *address.as_pointer<volatile u32>() = reg_as_u32[1];
        return;
    }
    case MMIOPolicy::IGNORE_UPPER_DWORD: {
        u32 reg_as_u32[2];
        copy_memory(&value, reg_as_u32, 2 * sizeof(u32));

        // We're not allowed to write the upper half, so assert that it's empty
        ASSERT(reg_as_u32[1] == 0);

        *address.as_pointer<volatile u32>() = reg_as_u32[0];
        return;
    }
    default:
        ASSERT_NEVER_REACHED();
    }
}

#ifdef ULTRA_32

#define SET_DWORDS_TO_ADDRESS(lower, upper, address) \
    do {                                             \
        lower = address & 0xFFFFFFFF;                \
        upper = 0;                                   \
    } while (0)

#define ADDRESS_FROM_TWO_DWORDS(lower, upper) \
    Address(lower);                           \
    ASSERT(upper == 0)

#elif defined(ULTRA_64)

#define SET_DWORDS_TO_ADDRESS(lower, upper, address) \
    do {                                             \
        lower = (address)&0xFFFFFFFF;                \
        upper = ((address) >> 32) & 0xFFFFFFFF;      \
    } while (0)

#define ADDRESS_FROM_TWO_DWORDS(lower, upper) Address((lower) | (static_cast<Address::underlying_pointer_type>(upper) << 32))

#endif

}
