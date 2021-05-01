#pragma once

#include "Common/Utilities.h"

namespace kernel {

template <typename First, typename Second>
class Pair {
public:
    Pair() = default;

    template <typename FirstT, typename SecondT>
    Pair(FirstT&& first, SecondT&& second)
        : first(forward<FirstT>(first))
        , second(forward<SecondT>(second))
    {
    }

    // TODO: SFINAE based on whether FirstT & SecondT are convertable to First & Second
    template <typename FirstT, typename SecondT>
    Pair(const Pair<FirstT, SecondT>& other)
        : first(other.first)
        , second(other.second)
    {
    }

    template <typename FirstT, typename SecondT>
    Pair(Pair<FirstT, SecondT>&& other)
        : first(move(other.first))
        , second(move(other.second))
    {
    }

    template <typename FirstT, typename SecondT>
    Pair& operator=(const Pair<FirstT, SecondT>& other)
    {
        first = other.first;
        second = other.second;

        return *this;
    }

    template <typename FirstT, typename SecondT>
    Pair& operator=(const Pair<FirstT, SecondT>&& other)
    {
        first = move(other.first);
        second = move(other.second);

        return *this;
    }

    First first;
    Second second;
};

template <typename First, typename Second, typename Third, typename Fourth>
class Quad {
public:
    template <typename FirstT, typename SecondT, typename ThirdT, typename FourthT>
    Quad(FirstT&& first, SecondT&& second, ThirdT&& third, FourthT&& fourth)
        : first(forward<FirstT>(first))
        , second(forward<SecondT>(second))
        , third(forward<ThirdT>(third))
        , fourth(forward<FourthT>(fourth))
    {
    }

    First first;
    Second second;
    Third third;
    Fourth fourth;
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
