#pragma once

#include "Common/Pair.h"
#include "Common/Types.h"

namespace kernel {

class Point : public Pair<size_t, size_t> {
public:
    Point() = default;
    Point(size_t x, size_t y)
        : Pair(x, y)
    {
    }

    bool operator==(const Point& other) const { return (x() == other.x()) && (y() == other.y()); }

    bool operator!=(const Point& other) const { return !(*this == other); }

    void move_by(size_t x, size_t y)
    {
        first() += x;
        second() += y;
    }

    size_t x() const { return first(); }
    size_t y() const { return second(); }
};
}
