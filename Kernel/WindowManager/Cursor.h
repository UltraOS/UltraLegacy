#pragma once

#include "Bitmap.h"
#include "Point.h"
#include "Rect.h"

namespace kernel {

class Cursor {
public:
    Cursor(Point location = { 0, 0 });

    void set_location(Point p) { m_location = p; }

    const Point& location() const { return m_location; }

    const BitmapView& bitmap() const { return m_bitmap; }

private:
    static constexpr size_t bitmap_height         = 19;
    static constexpr size_t bitmap_width          = 12;
    static constexpr size_t bitmap_width_in_bytes = 2;

    static const u8    s_bitmap_data[bitmap_height * bitmap_width_in_bytes];
    static const Color s_palette[2];

    BitmapView m_bitmap;
    Point      m_location;
};

}
