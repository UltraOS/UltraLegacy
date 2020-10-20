#pragma once

#include "Core/Runtime.h"
#include "Cursor.h"
#include "Drivers/Video/VideoDevice.h"
#include "Rect.h"

namespace kernel {

class Screen {
    MAKE_SINGLETON(Screen);

public:
    Screen(VideoDevice& device);

    static void initialize(VideoDevice& device) { s_instance = new Screen(device); }

    static Screen& the()
    {
        ASSERT(s_instance != nullptr);

        return *s_instance;
    }

    void recalculate_cursor_position(i16 delta_x, i16 delta_y);
    void check_if_focused_window_should_change();

    const Rect& rect() const { return m_rect; }

    const Cursor& cursor() const { return m_cursor; }

    Surface& surface() const { return m_device.surface(); }

    size_t width() const { return m_rect.width(); }
    size_t height() const { return m_rect.height(); }

private:
    VideoDevice& m_device;
    Cursor       m_cursor;
    Rect         m_rect;

    static Screen* s_instance;
};
}
