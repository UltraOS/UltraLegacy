#pragma once

#include "Color.h"
#include "Common/DynamicArray.h"
#include "Common/RefPtr.h"
#include "Core/Runtime.h"
#include "Painter.h"
#include "Rect.h"
#include "Window.h"

namespace kernel {

class Compositor {
public:
    static constexpr Color desktop_color = { 0x2d, 0x2d, 0x2d };
    static constexpr Color taskbar_color = { 0x8d, 0x26, 0x64 };

    static void run();

    static Compositor& the()
    {
        ASSERT(s_instance != nullptr);
        return *s_instance;
    }

    void compose();

private:
    Compositor();

    void prepare_desktop();
    void update_clock_widget();

    void update_cursor_position();

private:
    Painter* m_painter;

    Rect m_clock_rect;

    bool  m_cursor_invalidated { false };
    Point m_last_cursor_location;

    DynamicArray<Rect> m_dirty_rects;

    static Compositor* s_instance;
};
}
