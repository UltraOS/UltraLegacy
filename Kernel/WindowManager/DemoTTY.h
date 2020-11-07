#pragma once

#include "Common/RefPtr.h"
#include "Painter.h"
#include "Time/Time.h"
#include "Window.h"

namespace kernel {

class DemoTTY {
    MAKE_SINGLETON(DemoTTY);

public:
    static void initialize();

    static DemoTTY& the()
    {
        ASSERT(s_instance != nullptr);
        return *s_instance;
    }

    void write(StringView);
    void clear_characters(size_t count, bool draw_cursor = true);

    enum class Direction {
        UP,
        DOWN
    };

    void scroll(Direction);
    void clear_screen();

private:
    void draw_cursor();

    [[noreturn]] static void run();
    void tick();
    void execute_command();

private:
    static constexpr Rect window_rect = { 200, 200, 500, 300 };
    static constexpr Color background_color = { 0x1F, 0x1B, 0x24 };
    static constexpr StringView command_prompt = "> "_sv;

    // TODO: Replace with static constexpr Point once it supports constexpr ctors
    static constexpr ssize_t padding_x = 2;
    static constexpr ssize_t padding_y = 2;

    String m_current_command;
    RefPtr<Window> m_window;
    Painter m_painter;

    Rect m_view_rect;
    ssize_t m_width_in_char { 0 };
    ssize_t m_height_in_char { 0 };
    ssize_t m_x_offset { 0 };
    ssize_t m_y_offset { 0 };
    ssize_t m_prompt_y { 0 };

    Time::time_t m_last_cursor_blink_time { 0 };
    bool m_cursor_is_shown { false };

    static DemoTTY* s_instance;
};
}