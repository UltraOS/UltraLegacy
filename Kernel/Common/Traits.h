#pragma once

#include "Types.h"

namespace kernel {

struct false_value {
    static constexpr bool value = false;
};

struct true_value {
    static constexpr bool value = true;
};

template <typename T, T v>
struct integral_constant {
    static constexpr T value = v;
};

// ---- remove_const_volatile ----
template <typename T>
struct remove_const_volatile {
    using type = T;
};

template <typename T>
struct remove_const_volatile<const T> {
    using type = T;
};

template <typename T>
struct remove_const_volatile<volatile T> {
    using type = T;
};

template <typename T>
struct remove_const_volatile<const volatile T> {
    using type = T;
};

template <typename T>
using remove_const_volatile_t = typename remove_const_volatile<T>::type;
// --------------------

// ---- is_integral ----
// TODO: implement is_any_of and remove_const_volatile here
template <typename T>
struct is_integral : false_value {
};

template <>
struct is_integral<u8> : true_value {
};

template <>
struct is_integral<u16> : true_value {
};

template <>
struct is_integral<u32> : true_value {
};

template <>
struct is_integral<u64> : true_value {
};

template <>
struct is_integral<i8> : true_value {
};

template <>
struct is_integral<i16> : true_value {
};

template <>
struct is_integral<i32> : true_value {
};

template <>
struct is_integral<i64> : true_value {
};

template <>
struct is_integral<long> : true_value {
};

template <>
struct is_integral<unsigned long> : true_value {
};

template <typename T>
inline constexpr bool is_integral_v = is_integral<T>::value;
// --------------------

// ---- is_floating_point ----
template <typename T>
struct is_floating_point : false_value {
};

template <>
struct is_floating_point<float> : true_value {
};

template <>
struct is_floating_point<double> : true_value {
};

template <>
struct is_floating_point<long double> : true_value {
};

template <typename T>
inline constexpr bool is_floating_point_v = is_floating_point<T>::value;
// --------------------

// ---- is_arithmetic ----
template <typename T>
struct is_arithmetic : integral_constant<bool, is_floating_point_v<T> || is_integral_v<T>> {
};

template <typename T>
inline constexpr bool is_arithmetic_v = is_arithmetic<T>::value;
// --------------------

// ---- is_pointer ----
template <typename T>
struct is_actually_pointer : false_value {
};

template <typename T>
struct is_actually_pointer<T*> : true_value {
};

template <typename T>
struct is_pointer : is_actually_pointer<typename remove_const_volatile<T>::type> {
};

template <typename T>
inline constexpr bool is_pointer_v = is_pointer<T>::value;
// --------------------

// ---- is_trivially_constructible ----
template <typename T>
struct is_trivially_constructible : integral_constant<bool, __has_trivial_constructor(T)> {
};

template <typename T>
inline constexpr bool is_trivially_constructible_v = is_trivially_constructible<T>::value;
// --------------------

// ---- is_trivially_destructible ----
template <typename T>
struct is_trivially_destructible : integral_constant<bool, __has_trivial_destructor(T)> {
};

template <typename T>
inline constexpr bool is_trivially_destructible_v = is_trivially_destructible<T>::value;
// --------------------

// ---- is_trivially_copyable ----
template <typename T>
struct is_trivially_copyable : integral_constant<bool, __has_trivial_copy(T)> {
};

template <typename T>
inline constexpr bool is_trivially_copyable_v = is_trivially_copyable<T>::value;
// --------------------

// ---- is_same ----
template <typename T, typename U>
struct is_same : false_value {
};

template <typename T>
struct is_same<T, T> : true_value {
};

template <typename T, typename U>
inline constexpr bool is_same_v = is_same<T, U>::value;
// --------------------

// ---- remove_reference ----
template <typename T>
struct remove_reference {
    using type = T;
};

template <typename T>
struct remove_reference<T&> {
    using type = T;
};

template <typename T>
struct remove_reference<T&&> {
    using type = T;
};

template <typename T>
using remove_reference_t = typename remove_reference<T>::type;
// --------------------

// ---- enable_if ----
template <bool value, typename T = void>
struct enable_if {
};

template <typename T>
struct enable_if<true, T> {
    using type = T;
};

template <bool value, typename T = void>
using enable_if_t = typename enable_if<value, T>::type;
// --------------------
}
