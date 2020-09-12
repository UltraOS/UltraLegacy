#pragma once

#include "Utilities.h"

namespace kernel {

class Cursor {
public:
    Cursor(Point location = { 0, 0 });

    void set_location(Point p) { m_location = p; }

    const Point& location() const { return m_location; }

    const u16* bitmap() const { return s_bitmap; }

private:
    static const u16 s_bitmap[19];

    Point m_location;
    Rect  m_rect;
};

}
