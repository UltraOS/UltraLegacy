#pragma once

#include "Core/Runtime.h"
#include "Drivers/Video/Utilities.h"

namespace kernel {

class Compositor {
public:
    static void run();

private:
    static void draw_desktop();
    static void draw_clock_widget();

    static inline Rect s_desktop_rect;
    static inline Rect s_taskbar_rect;

    static constexpr Color desktop_color = { 0x2d, 0x2d, 0x2d };
    static constexpr Color taskbar_color = { 0x8d, 0x26, 0x64 };
};
}
