#pragma once

#include "Common/Pair.h"
#include "Common/Types.h"

namespace kernel {

template <typename T>
class BasicPoint : public Pair<T, T> {
public:
    BasicPoint() = default;
    BasicPoint(T x, T y)
        : Pair<T, T>(x, y)
    {
    }

    bool operator==(const BasicPoint& other) const { return (x() == other.x()) && (y() == other.y()); }

    bool operator!=(const BasicPoint& other) const { return !(*this == other); }

    void move_by(T x, T y)
    {
        this->first() += x;
        this->second() += y;
    }

    T x() const { return this->first(); }
    T y() const { return this->second(); }
};

using Point = BasicPoint<ssize_t>;
}
