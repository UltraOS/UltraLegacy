#include "Compositor.h"
#include "Drivers/Video/VideoDevice.h"
#include "Multitasking/Sleep.h"

#include "Core/CPU.h"

#include "Time/Time.h"

namespace kernel {

void Compositor::run()
{
    auto desktop_height = VideoDevice::the().height() * 97 / 100;
    auto bar_height     = VideoDevice::the().height() - desktop_height;

    Color color { 0x8d, 0x26, 0x64 };
    VideoDevice::the().fill_rect(Rect(0, 0, VideoDevice::the().width(), desktop_height), { 0x2d, 0x2d, 0x2d });
    VideoDevice::the().fill_rect(Rect(0, desktop_height, VideoDevice::the().width(), bar_height), color);

    for (;;) {
        redraw_clock_widget();
        sleep::for_milliseconds(1000 / 60);
    }
}

void Compositor::redraw_clock_widget()
{
    static Time::time_t last_drawn_second = 0;

    if (Time::now() == last_drawn_second)
        return;

    last_drawn_second = Time::now();

    Color color { 0x8d, 0x26, 0x64 };

    Color other_color = Color::white();

    auto time  = Time::now_readable();
    bool is_pm = false;

    char digit[8];

    if (time.hour >= 12) {
        is_pm = true;

        if (time.hour != 12)
            time.hour -= 12;
    }

    to_string(time.hour, digit, 8);
    if (time.hour < 10) {
        digit[1] = digit[0];
        digit[0] = '0';
    }

    VideoDevice::the().draw_char({ 900 + 25, 747 }, digit[0], other_color, color);
    VideoDevice::the().draw_char({ 908 + 25, 747 }, digit[1], other_color, color);
    VideoDevice::the().draw_char({ 916 + 25, 747 }, ':', other_color, color);

    to_string(time.minute, digit, 8);
    if (time.minute < 10) {
        digit[1] = digit[0];
        digit[0] = '0';
    }

    VideoDevice::the().draw_char({ 924 + 25, 747 }, digit[0], other_color, color);
    VideoDevice::the().draw_char({ 932 + 25, 747 }, digit[1], other_color, color);
    VideoDevice::the().draw_char({ 940 + 25, 747 }, ':', other_color, color);

    to_string(time.second, digit, 8);
    if (time.second < 10) {
        digit[1] = digit[0];
        digit[0] = '0';
    }

    VideoDevice::the().draw_char({ 948 + 25, 747 }, digit[0], other_color, color);
    VideoDevice::the().draw_char({ 956 + 25, 747 }, digit[1], other_color, color);

    VideoDevice::the().draw_char({ 972 + 25, 747 }, is_pm ? 'P' : 'A', other_color, color);
    VideoDevice::the().draw_char({ 980 + 25, 747 }, 'M', other_color, color);
}
}
