#pragma once

#include "Traits.h"

namespace kernel {

template <typename T>
enable_if_t<is_integral_v<T>, T> ceiling_divide(T l, T r)
{
    return !!l + ((l - !!l) / r);
}
}
