#pragma once

#include "Common/Pair.h"
#include "Common/Types.h"

namespace kernel {

template <typename T>
class BasicPoint : public Pair<T, T> {
public:
    constexpr BasicPoint() = default;
    constexpr BasicPoint(T x, T y)
        : Pair<T, T>(x, y)
    {
    }

    constexpr bool operator==(const BasicPoint& other) const { return (x() == other.x()) && (y() == other.y()); }

    constexpr bool operator!=(const BasicPoint& other) const { return !(*this == other); }

    constexpr void move_by(T x, T y)
    {
        this->first += x;
        this->second += y;
    }

    constexpr void move_by(BasicPoint other)
    {
        this->first += other.first;
        this->second += other.second;
    }

    constexpr BasicPoint moved_by(BasicPoint other)
    {
        BasicPoint new_point = *this;
        new_point.move_by(other);
        return new_point;
    }

    constexpr BasicPoint operator-() const
    {
        return BasicPoint(-this->first, -this->second);
    }

    constexpr T x() const { return this->first; }
    constexpr T y() const { return this->second; }
};

using Point = BasicPoint<ssize_t>;
}
