#include "Compositor.h"
#include "Drivers/Video/VideoDevice.h"
#include "Multitasking/Sleep.h"
#include "Screen.h"

#include "Core/CPU.h"

#include "Time/Time.h"

namespace kernel {

void Compositor::run()
{
    s_painter = new Painter();

    auto desktop_height = Screen::the().height() * 97 / 100;
    auto taskbar_height = Screen::the().height() - desktop_height;

    s_desktop_rect.set_width(Screen::the().width());
    s_desktop_rect.set_height(desktop_height);
    s_desktop_rect.set_top_left_x(0);
    s_desktop_rect.set_top_left_y(0);

    s_taskbar_rect.set_width(Screen::the().width());
    s_taskbar_rect.set_height(taskbar_height);
    s_taskbar_rect.set_top_left_x(0);
    s_taskbar_rect.set_top_left_y(desktop_height);

    s_clock_top_left.set_left(Screen::the().width() - 100);
    s_clock_top_left.set_right(s_taskbar_rect.center().right() - 8);

    draw_desktop();

    s_painter->draw_bitmap(const_cast<u16*>(Screen::the().cursor().bitmap()),
                           19,
                           Screen::the().cursor().location(),
                           Color::white(),
                           desktop_color);

    for (;;) {
        draw_clock_widget();

        if (Screen::the().cursor().location() != s_last_cursor_location) {
            s_painter->fill_rect(Rect(s_last_cursor_location.x(), s_last_cursor_location.y(), 16, 19), desktop_color);

            s_last_cursor_location = Screen::the().cursor().location();

            s_painter->draw_bitmap(const_cast<u16*>(Screen::the().cursor().bitmap()),
                                   19,
                                   Screen::the().cursor().location(),
                                   Color::white(),
                                   desktop_color);
        }

        sleep::for_milliseconds(1000 / 60);
    }
}

void Compositor::draw_desktop()
{
    s_painter->fill_rect(s_desktop_rect, desktop_color);
    s_painter->fill_rect(s_taskbar_rect, taskbar_color);
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

    size_t current_x_offset = s_clock_top_left.x();

    auto draw_digits = [&current_x_offset](char* digits, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            s_painter->draw_char({ current_x_offset, s_clock_top_left.y() }, digits[i], digit_color, taskbar_color);
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
