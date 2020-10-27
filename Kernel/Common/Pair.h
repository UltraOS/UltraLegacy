#pragma once

#include "Common/Utilities.h"

namespace kernel {

template <typename First, typename Second>
class Pair {
public:
    Pair() = default;

    template <typename FirstT, typename SecondT>
    Pair(FirstT&& first, SecondT&& second)
        : m_first(forward<FirstT>(first))
        , m_second(forward<SecondT>(second))
    {
    }

    First& first() { return m_first; }
    const First& first() const { return m_first; }

    Second& second() { return m_second; }
    const Second& second() const { return m_second; }

    void set_first(const First& first) { m_first = first; }
    void set_second(const Second& second) { m_second = second; }

private:
    First m_first;
    Second m_second;
};

template <typename First, typename Second, typename Third, typename Fourth>
class Quad {
public:
    template <typename FirstT, typename SecondT, typename ThirdT, typename FourthT>
    Quad(FirstT&& first, SecondT&& second, ThirdT&& third, FourthT&& fourth)
        : m_first(forward<FirstT>(first))
        , m_second(forward<SecondT>(second))
        , m_third(forward<ThirdT>(third))
        , m_fourth(forward<FourthT>(fourth))
    {
    }

    First& first() { return m_first; }
    const First& first() const { return m_first; }

    Second& second() { return m_second; }
    const Second& second() const { return m_second; }

    Third& third() { return m_third; }
    const Third& third() const { return m_third; }

    Fourth& fourth() { return m_fourth; }
    const Fourth& fourth() const { return m_fourth; }

private:
    First m_first;
    Second m_second;
    Third m_third;
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
