#pragma once

#include "Core/Runtime.h"
#include "Painter.h"
#include "Utilities.h"

namespace kernel {

class Compositor {
public:
    static void run();

    static void on_cursor_moved(i16 delta_x, i16 delta_y);

private:
    static void draw_desktop();
    static void draw_clock_widget();

    static inline Painter* s_painter;

    static inline Rect s_desktop_rect;
    static inline Rect s_taskbar_rect;

    static inline Point s_clock_top_left;

    static inline Point s_last_cursor_location;

    static constexpr Color desktop_color = { 0x2d, 0x2d, 0x2d };
    static constexpr Color taskbar_color = { 0x8d, 0x26, 0x64 };
};
}
