#pragma once

#include "Common/DynamicArray.h"
#include "Core/Runtime.h"
#include "Painter.h"
#include "Utilities.h"

namespace kernel {

class Compositor {
public:
    static void run();

    static Compositor& the()
    {
        ASSERT(s_instance != nullptr);
        return *s_instance;
    }

    void compose();

private:
    Compositor();

    void draw_desktop();
    void draw_clock_widget();

    void check_if_cursor_moved();

private:
    Painter* m_painter;

    Rect m_desktop_rect;
    Rect m_taskbar_rect;

    Point m_clock_top_left;

    bool  m_cursor_invalidated { false };
    Point m_last_cursor_location;

    DynamicArray<Rect> m_dirty_rects;

    static Compositor* s_instance;

    static constexpr Color desktop_color = { 0x2d, 0x2d, 0x2d };
    static constexpr Color taskbar_color = { 0x8d, 0x26, 0x64 };
};
}
