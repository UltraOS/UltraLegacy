#pragma once

#include "Memory.h"
#include "Traits.h"

namespace kernel {

template <typename T>
T&& forward(remove_reference_t<T>& value)
{
    return static_cast<T&&>(value);
}

template <typename T>
T&& forward(remove_reference_t<T>&& value)
{
    // TODO: assert that we're not forwarding an rvalue as an lvalue here
    return static_cast<T&&>(value);
}

template <typename T>
remove_reference_t<T>&& move(T&& value)
{
    return static_cast<remove_reference_t<T>&&>(value);
}

template <typename T>
constexpr const T& max(const T& l, const T& r)
{
    return l < r ? r : l;
}

template <typename T>
constexpr const T& min(const T& l, const T& r)
{
    return l < r ? l : r;
}

template <typename T>
void swap(T& l, T& r)
{
    T tmp = move(l);
    l = move(r);
    r = move(tmp);
}

// clang-format off
template <typename To, typename From>
enable_if_t<
    sizeof(To) == sizeof(From) &&
    is_trivially_constructible_v<To> &&
    is_trivially_copyable_v<To> &&
    is_trivially_copyable_v<From>,
    To
>
bit_cast(From value)
{
    To to;
    copy_memory(&value, &to, sizeof(value));
    return to;
}
// clang-format on

template <typename T>
struct Less {
    constexpr bool operator()(const T& l, const T& r) const
    {
        return l < r;
    }
};

template <typename T>
struct Greater {
    constexpr bool operator()(const T& l, const T& r) const
    {
        return l > r;
    }
};

template <typename T, typename Compare = Less<T>>
T* lower_bound(T* begin, T* end, const T& value, Compare comparator = Compare())
{
    if (begin == end)
        return end;

    auto* lower_bound = end;

    ssize_t left = 0;
    ssize_t right = end - begin - 1;

    while (left <= right) {
        ssize_t pivot = left + (right - left) / 2;

        auto& pivot_value = begin[pivot];

        if (comparator(value, pivot_value)) {
            lower_bound = &pivot_value;
            right = pivot - 1;
            continue;
        }

        if (comparator(pivot_value, value)) {
            left = pivot + 1;
            continue;
        }

        return &begin[pivot];
    }

    return lower_bound;
}

template <typename T, typename Compare = Less<T>>
T* binary_search(T* begin, T* end, const T& value, Compare comparator = Compare())
{
    auto* result = lower_bound(begin, end, value, comparator);

    return result == end ? result : (comparator(value, *result) ? end : result);
}
}
