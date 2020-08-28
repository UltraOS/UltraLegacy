#pragma once

#include "Common/Utilities.h"

namespace kernel {

template <typename Left, typename Right>
class Pair {
public:
    Pair() = default;

    template <typename LeftT, typename RightT>
    Pair(LeftT&& left, RightT&& right) : m_left(forward<LeftT>(left)), m_right(forward<RightT>(right))
    {
    }

    Left&       left() { return m_left; }
    const Left& left() const { return m_left; }

    Right&       right() { return m_right; }
    const Right& right() const { return m_right; }

    void set_left(const Left& left) { m_left = left; }
    void set_right(const Right& right) { m_right = right; }

private:
    Left  m_left;
    Right m_right;
};

template <typename First, typename Second, typename Third, typename Fourth>
class Quad {
public:
    template <typename FirstT, typename SecondT, typename ThirdT, typename FourthT>
    Quad(FirstT&& first, SecondT&& second, ThirdT&& third, FourthT&& fourth)
        : m_first(forward<FirstT>(first)), m_second(forward<SecondT>(second)), m_third(forward<ThirdT>(third)),
          m_fourth(forward<FourthT>(fourth))
    {
    }

    First&       first() { return m_first; }
    const First& first() const { return m_first; }

    Second&       second() { return m_second; }
    const Second& second() const { return m_second; }

    Third&       third() { return m_third; }
    const Third& third() const { return m_third; }

    Fourth&       fourth() { return m_fourth; }
    const Fourth& fourth() const { return m_fourth; }

private:
    First  m_first;
    Second m_second;
    Third  m_third;
    Fourth m_fourth;
};

template <typename Left, typename Right>
Pair<remove_reference_t<Left>, remove_reference_t<Right>> make_pair(Left&& left, Right&& right)
{
    using PairType = Pair<remove_reference_t<Left>, remove_reference_t<Right>>;

    return PairType(forward<Left>(left), forward<Right>(right));
}

template <typename First, typename Second, typename Third, typename Fourth>
auto make_quad(First&& first, Second&& second, Third&& third, Fourth&& fourth)
{
    // TODO: this should really be decay_t
    using QuadType = Quad<remove_reference_t<First>,
                          remove_reference_t<Second>,
                          remove_reference_t<Third>,
                          remove_reference_t<Fourth>>;

    return QuadType(forward<First>(first), forward<Second>(second), forward<Third>(third), forward<Fourth>(fourth));
}
}
