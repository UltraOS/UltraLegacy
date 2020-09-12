#pragma once

#include "Core/Boot.h"
#include "Utilities.h"

namespace kernel {

class Painter {
public:
    Painter();

    void draw_bitmap(const u8* map, size_t height, Point top_left, Color char_color, Color fill_color);
    void draw_bitmap(const u16* map, size_t height, Point top_left, Color char_color, Color fill_color);
    void fill_rect(const Rect&, Color color);
    void draw_at(size_t x, size_t y, Color pixel);
    void draw_char(Point top_left, char, Color char_color, Color fill_color);

private:
    VideoMode m_mode;

    static constexpr size_t font_height = 16;
    static constexpr size_t font_width  = 8;
    static const u8         s_font[256][font_height];
};
}
