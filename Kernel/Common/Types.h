#pragma once
namespace kernel {

// signed types
using i8  = char;
using i16 = short int;
using i32 = int;
using i64 = long long int;

// unsigned types
using u8  = unsigned char;
using u16 = unsigned short int;
using u32 = unsigned int;
using u64 = unsigned long long int;

using size_t = long unsigned int;

// A wild posix type appears
using ssize_t = i32;

using ptr_t = u32;

// floating point
using f32 = float;
using f64 = double;

#define SIZEOF_I8  1
#define SIZEOF_I16 2
#define SIZEOF_I32 4
#define SIZEOF_I64 8

#define SIZEOF_U8  1
#define SIZEOF_U16 2
#define SIZEOF_U32 4
#define SIZEOF_U64 8

#define SIZEOF_F32 4
#define SIZEOF_F64 8

static_assert(sizeof(i8) == SIZEOF_I8, "Incorrect size of 8 bit integer");
static_assert(sizeof(i16) == SIZEOF_I16, "Incorrect size of 16 bit integer");
static_assert(sizeof(i32) == SIZEOF_I32, "Incorrect size of 32 bit integer");
static_assert(sizeof(i64) == SIZEOF_I64, "Incorrect size of 64 bit integer");

static_assert(sizeof(u8) == SIZEOF_U8, "Incorrect size of 8 bit unsigned integer");
static_assert(sizeof(u16) == SIZEOF_U16, "Incorrect size of 16 bit unsigned integer");
static_assert(sizeof(u32) == SIZEOF_U32, "Incorrect size of 32 bit unsigned integer");
static_assert(sizeof(u64) == SIZEOF_U64, "Incorrect size of 64 bit unsigned integer");

static_assert(sizeof(f32) == SIZEOF_F32, "Incorrect size of 32 bit float");
static_assert(sizeof(f64) == SIZEOF_F64, "Incorrect size of 64 bit float");

#undef SIZEOF_F32
#undef SIZEOF_F64

#undef SIZEOF_U8
#undef SIZEOF_U16
#undef SIZEOF_U32
#undef SIZEOF_U64

#undef SIZEOF_I8
#undef SIZEOF_I16
#undef SIZEOF_I32
#undef SIZEOF_I64

class Address {
public:
    Address() = default;

    template <typename T>
    constexpr Address(T* ptr) : m_ptr(reinterpret_cast<ptr_t>(ptr))
    {
    }

    constexpr Address(decltype(nullptr)) : m_ptr(0) { }

    constexpr Address(ptr_t address) : m_ptr(address) { }

    constexpr ptr_t raw() const { return m_ptr; }

    template <typename T>
    constexpr T* as_pointer()
    {
        return reinterpret_cast<T*>(m_ptr);
    }

    template <typename T>
    constexpr T* as_pointer() const
    {
        return const_cast<T*>(reinterpret_cast<const T*>(m_ptr));
    }

    constexpr operator ptr_t() const { return m_ptr; }

private:
    ptr_t m_ptr;
};
}
