#pragma once

#include "Core/Runtime.h"
#include "Cursor.h"
#include "Utilities.h"

namespace kernel {

class Screen {
public:
    Screen();

    static void initialize() { s_instance = new Screen(); }

    static Screen& the()
    {
        ASSERT(s_instance != nullptr);

        return *s_instance;
    }

    void recalculate_cursor_position(i16 delta_x, i16 delta_y);

    const Rect& rect() const { return m_rect; }

    const Cursor& cursor() const { return m_cursor; }

    size_t width() const { return m_rect.width(); }
    size_t height() const { return m_rect.height(); }

private:
    Cursor m_cursor;
    Rect   m_rect;

    static Screen* s_instance;
};
}
