#pragma once

#include "Color.h"
#include "Rect.h"
#include "Bitmap.h"

namespace kernel {

class Window;

class WindowFrame {
public:
    static constexpr size_t upper_frame_height  = 20;
    static constexpr size_t close_button_width = 20;
    static constexpr size_t minimize_button_width = 20;

    static constexpr Color  default_frame_color = { 0x8d, 0x26, 0x64 };

    WindowFrame(Window& owner);
    void paint();
    Rect rect() const;
    Rect view_rect() const;

    Rect draggable_rect() const;
    Rect close_button_rect() const;
    Rect minimize_button_rect() const;

    void on_close_button_hovered();
    void on_close_button_released();

private:
    Rect raw_close_button_rect() const;
    void draw_close_button(Color fill);

private:
    Window& m_owner;
    BitmapView m_close_button_bitmap;

    static constexpr size_t close_button_bitmap_height         = 8;
    static constexpr size_t close_button_bitmap_width          = 8;
    static constexpr size_t close_button_bitmap_width_in_bytes = 2;

    static const u8    s_close_button_bitmap_data[close_button_bitmap_height * close_button_bitmap_width];
    static const Color s_palette[2];
};

}
