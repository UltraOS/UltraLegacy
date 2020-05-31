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

template<bool value, typename T = void>
struct enable_if {};

template<typename T>
struct enable_if<true, T>
{
    using type = T;
};

template <bool value, typename T = void>
using enable_if_t = typename enable_if<value, T>::type;
