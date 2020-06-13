#pragma once

#include "Traits.h"

template<typename T>
T&& forward(remove_reference_t<T>& value)
{
    return static_cast<T&&>(value);
}

template<typename T>
T&& forward(remove_reference_t<T>&& value)
{
    // TODO: assert that we're not forwarding an rvalue as an lvalue here
    return static_cast<T&&>(value);
}

template<typename T>
remove_reference_t<T>&& move(T&& value)
{
    return static_cast<remove_reference_t<T>&&>(value);
}

template <typename T>
constexpr const T& max(const T& l, const T& r)
{
    return l < r ? r : l;
}
