#include "Compositor.h"
#include "Drivers/Video/VideoDevice.h"
#include "Multitasking/Sleep.h"
#include "Screen.h"

#include "Core/CPU.h"

#include "Time/Time.h"

namespace kernel {

Compositor* Compositor::s_instance;

Compositor::Compositor() : m_painter(new Painter(Screen::the().surface()))
{
    auto desktop_height = Screen::the().height() * 97 / 100;
    auto taskbar_height = Screen::the().height() - desktop_height;

    m_desktop_rect.set_width(Screen::the().width());
    m_desktop_rect.set_height(desktop_height);
    m_desktop_rect.set_top_left_x(0);
    m_desktop_rect.set_top_left_y(0);

    m_taskbar_rect.set_width(Screen::the().width());
    m_taskbar_rect.set_height(taskbar_height);
    m_taskbar_rect.set_top_left_x(0);
    m_taskbar_rect.set_top_left_y(desktop_height);

    m_clock_top_left.set_left(Screen::the().width() - 100);
    m_clock_top_left.set_right(m_taskbar_rect.center().right() - 8);

    const auto& cursor     = Screen::the().cursor();
    m_last_cursor_location = cursor.location();

    draw_desktop();
    m_painter->draw_bitmap(cursor.bitmap(), m_last_cursor_location);
}

void Compositor::compose()
{
    draw_clock_widget();
    check_if_cursor_moved();

    for (auto& rect: m_dirty_rects)
        m_painter->fill_rect(rect, desktop_color);

    m_dirty_rects.clear();

    if (m_cursor_invalidated) {
        m_cursor_invalidated = false;
        m_painter->draw_bitmap(Screen::the().cursor().bitmap(), m_last_cursor_location);
    }
}

void Compositor::run()
{
    s_instance = new Compositor();

    for (;;) {
        Compositor::the().compose();
        sleep::for_milliseconds(1000 / 100);
    }
}

void Compositor::check_if_cursor_moved()
{
    auto& cursor   = Screen::the().cursor();
    auto  location = cursor.location();

    if (location != m_last_cursor_location) {
        m_dirty_rects.emplace(m_last_cursor_location, cursor.bitmap().width(), cursor.bitmap().height());
        m_cursor_invalidated   = true;
        m_last_cursor_location = location;
    }
}

void Compositor::draw_desktop()
{
    m_painter->fill_rect(m_desktop_rect, desktop_color);
    m_painter->fill_rect(m_taskbar_rect, taskbar_color);
}

void Compositor::draw_clock_widget()
{
    static constexpr Color digit_color = Color::white();

    static Time::time_t last_drawn_second = 0;

    if (Time::now() == last_drawn_second)
        return;

    last_drawn_second = Time::now();

    auto time  = Time::now_readable();
    bool is_pm = false;

    if (time.hour >= 12) {
        is_pm = true;

        if (time.hour != 12)
            time.hour -= 12;
    } else if (time.hour == 0)
        time.hour = 12;

    // hardcoded for now
    static constexpr size_t font_width = 8;

    size_t current_x_offset = m_clock_top_left.x();

    auto draw_digits = [&](char* digits, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            m_painter->draw_char({ current_x_offset, m_clock_top_left.y() }, digits[i], digit_color, taskbar_color);
            current_x_offset += font_width;
        }
    };

    char digits[8];

    to_string(time.hour, digits, 8);
    if (time.hour < 10) {
        digits[1] = digits[0];
        digits[0] = '0';
    }

    digits[2] = ':';

    draw_digits(digits, 3);

    to_string(time.minute, digits, 8);
    if (time.minute < 10) {
        digits[1] = digits[0];
        digits[0] = '0';
    }
    digits[2] = ':';

    draw_digits(digits, 3);

    to_string(time.second, digits, 8);
    if (time.second < 10) {
        digits[1] = digits[0];
        digits[0] = '0';
    }
    digits[2] = ' ';

    draw_digits(digits, 3);

    digits[0] = is_pm ? 'P' : 'A';
    digits[1] = 'M';

    draw_digits(digits, 2);
}
}
