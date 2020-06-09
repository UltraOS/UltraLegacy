#pragma once

#include "Types.h"

struct false_value
{
    static constexpr bool value = false;
};

struct true_value
{
    static constexpr bool value = true;
};

// ---- remove_const_volatile ----

template<typename T>
struct remove_const_volatile
{
    using type = T;
};

template<typename T>
struct remove_const_volatile<const T>
{
    using type = T;
};

template<typename T>
struct remove_const_volatile<volatile T>
{
    using type = T;
};

template<typename T>
struct remove_const_volatile<const volatile T>
{
    using type = T;
};

template<typename T>
using remove_const_volatile_t = typename remove_const_volatile<T>::type;

// --------------------

// ---- is_integral ----
// TODO: implement is_any_of and remove_const_volatile here

template<typename T>
struct is_integral : false_value {};

template<>
struct is_integral<u8> : true_value {};

template<>
struct is_integral<u16> : true_value {};

template<>
struct is_integral<u32> : true_value {};

template<>
struct is_integral<u64> : true_value {};

template<>
struct is_integral<i8> : true_value {};

template<>
struct is_integral<i16> : true_value {};

template<>
struct is_integral<i32> : true_value {};

template<>
struct is_integral<i64> : true_value {};

template<>
struct is_integral<long> : true_value {};

template<>
struct is_integral<unsigned long> : true_value {};

template<typename T>
inline constexpr bool is_integral_v = is_integral<T>::value;

// --------------------

// ---- is_pointer ----

template<typename T>
struct is_actually_pointer : false_value {};

template<typename T>
struct is_actually_pointer<T*> : true_value {};

template<typename T>
struct is_pointer : is_actually_pointer<typename remove_const_volatile<T>::type>
{
};

template<typename T>
inline constexpr bool is_pointer_v = is_pointer<T>::value;

// --------------------


// ---- enable_if ----

template<bool value, typename T = void>
struct enable_if {};

template<typename T>
struct enable_if<true, T>
{
    using type = T;
};

template <bool value, typename T = void>
using enable_if_t = typename enable_if<value, T>::type;

// --------------------
