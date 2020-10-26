#pragma once

#include "Core/Runtime.h"
#include "Theme.h"

namespace kernel {

class ClassicTheme final : public Theme {
public:
    ClassicTheme();

    Color desktop_background_color() const override { return { 0x2d, 0x2d, 0x2d }; }
    Color window_frame_color() const override { return taskbar_color(); }
    Color taskbar_color() const override { return { 0x8d, 0x26, 0x64 }; }

    size_t width_for_button(Button) const override { return 20; }

    Color color_for_button_state(Button button, ButtonState state) const override
    {
        static constexpr Color dimmer_frame_color = { 0x69, 0x1c, 0x4a };

        switch (button) {
        case Button::CLOSE:
            if (state == ButtonState::HOVERED)
                return Color::red();
            [[fallthrough]];
        default:
            if (state == ButtonState::HOVERED)
                return dimmer_frame_color;
            else
                return window_frame_color();
        }
    }

    const BitmapView& button_bitmap(Button button) const override
    {
        switch (button) {
        case Button::CLOSE: return m_close_button_bitmap;
        case Button::MAXIMIZE: return m_maximize_button_bitmap;
        case Button::MINIMIZE: return m_minimize_button_bitmap;
        default: ASSERT_NEVER_REACHED();
        }
    }

    const BitmapView& cursor_bitmap() const override { return m_cursor_bitmap; }

    size_t upper_window_frame_height() const override { return 20; }

private:
    static constexpr size_t cursor_bitmap_height         = 19;
    static constexpr size_t cursor_bitmap_width          = 12;
    static constexpr size_t cursor_bitmap_width_in_bytes = 2;

    static constexpr size_t close_button_bitmap_height         = 8;
    static constexpr size_t close_button_bitmap_width          = 8;
    static constexpr size_t close_button_bitmap_width_in_bytes = 2;

    static constexpr size_t maximize_button_bitmap_height         = 8;
    static constexpr size_t maximize_button_bitmap_width          = 8;
    static constexpr size_t maximize_button_bitmap_width_in_bytes = 2;

    static constexpr size_t minimize_button_bitmap_height         = 8;
    static constexpr size_t minimize_button_bitmap_width          = 8;
    static constexpr size_t minimize_button_bitmap_width_in_bytes = 2;

    static const u8 s_cursor_bitmap_data[cursor_bitmap_width * cursor_bitmap_height];
    static const u8 s_close_button_bitmap_data[close_button_bitmap_width * close_button_bitmap_height];
    static const u8 s_maximize_button_bitmap_data[maximize_button_bitmap_width * maximize_button_bitmap_height];
    static const u8 s_minimize_button_bitmap_data[minimize_button_bitmap_width * minimize_button_bitmap_height];

    static const Color s_palette[2];

    BitmapView m_cursor_bitmap;
    BitmapView m_close_button_bitmap;
    BitmapView m_maximize_button_bitmap;
    BitmapView m_minimize_button_bitmap;
};
}
