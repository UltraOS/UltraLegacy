#pragma once

#include "Bitmap.h"
#include "Common/Lock.h"
#include "Point.h"
#include "Rect.h"

namespace kernel {

class Cursor {
    MAKE_NONCOPYABLE(Cursor);

public:
    Cursor(Point location = { 0, 0 });

    void set_location(Point p)
    {
        LockGuard lock_guard(m_lock);
        m_location = p;
    }

    Point location() const
    {
        LockGuard lock_guard(m_lock);
        return m_location;
    }

    const Rect& rect() const { return m_rect; }

    const BitmapView& bitmap() const { return m_bitmap; }

private:
    static constexpr size_t bitmap_height         = 19;
    static constexpr size_t bitmap_width          = 12;
    static constexpr size_t bitmap_width_in_bytes = 2;

    static const u8    s_bitmap_data[bitmap_height * bitmap_width_in_bytes];
    static const Color s_palette[2];

    mutable InterruptSafeSpinLock m_lock;
    BitmapView                    m_bitmap;
    Rect                          m_rect;
    Point                         m_location;
};

}
