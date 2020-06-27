#pragma once

#include "Common/Utilities.h"

namespace kernel {

template <typename Left, typename Right>
class Pair {
public:
    Pair(Left&& l, Right&& r) : m_l(forward<Left>(l)), m_r(forward<Right>(r)) { }

    Left& left() { return m_l; }

    Right& right() { return m_r; }

    const Left& left() const { return m_l; }

    const Right& right() const { return m_r; }

private:
    Left  m_l;
    Right m_r;
};

template <typename Left, typename Right>
Pair<Left, Right> make_pair(Left&& l, Right&& r)
{
    return Pair(forward<Left>(l), forward<Right>(r));
}
}
