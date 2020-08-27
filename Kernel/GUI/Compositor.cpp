#include "Compositor.h"
#include "Drivers/Video/VideoDevice.h"
#include "Multitasking/Sleep.h"

#include "Core/CPU.h"

#include "Time/Time.h"

namespace kernel {

void Compositor::run()
{
    auto desktop_height = VideoDevice::the().height() * 97 / 100;
    auto taskbar_height = VideoDevice::the().height() - desktop_height;

    s_desktop_rect.set_width(VideoDevice::the().width());
    s_desktop_rect.set_height(desktop_height);
    s_desktop_rect.set_top_left_x(0);
    s_desktop_rect.set_top_left_y(0);

    s_taskbar_rect.set_width(VideoDevice::the().width());
    s_taskbar_rect.set_height(taskbar_height);
    s_taskbar_rect.set_top_left_x(0);
    s_taskbar_rect.set_top_left_y(desktop_height);

    draw_desktop();

    for (;;) {
        draw_clock_widget();
        sleep::for_milliseconds(1000 / 60);
    }
}

void Compositor::draw_desktop()
{
    VideoDevice::the().fill_rect(s_desktop_rect, desktop_color);
    VideoDevice::the().fill_rect(s_taskbar_rect, taskbar_color);
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
    }

    // hardcoded for now
    static constexpr size_t clock_y    = 747;
    static constexpr size_t clock_x    = 925;
    static constexpr size_t font_width = 8;

    size_t current_x_offset = clock_x;

    auto draw_digits = [&current_x_offset](char* digits, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            VideoDevice::the().draw_char({ current_x_offset, clock_y }, digits[i], digit_color, taskbar_color);
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

    current_x_offset = clock_x;
}
}
