#pragma once

#include "Bitmap.h"
#include "Common/Lock.h"
#include "Point.h"
#include "Rect.h"
#include "WindowManager.h"

namespace kernel {

class Cursor {
    MAKE_NONCOPYABLE(Cursor);

public:
    Cursor(Point location = { 0, 0 }) : m_location(location) { }

    void set_location(Point p) { m_location = p; }

    Point location() const { return m_location; }

    const BitmapView& bitmap() const { return WindowManager::the().active_theme()->cursor_bitmap(); }

    Rect rect() const
    {
        auto& bm = bitmap();

        return { 0, 0, bm.width(), bm.height() };
    }

private:
    Point m_location;
};
}
