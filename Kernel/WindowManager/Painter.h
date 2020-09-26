#pragma once

#include "Bitmap.h"
#include "Core/Boot.h"
#include "Point.h"
#include "Rect.h"

namespace kernel {

class Painter {
public:
    Painter(Surface* surface);

    void draw_bitmap(const Bitmap&, const Point& location);
    void blit(const Point& location, const Bitmap&, const Rect& source_rect);

    void fill_rect(const Rect&, Color color);
    void draw_at(size_t x, size_t y, Color pixel);
    void draw_char(Point top_left, char, Color char_color, Color fill_color);

    void set_clip_rect(const Rect& rect) { m_clip_rect = rect; }
    void reset_clip_rect() { set_clip_rect({ 0, 0, m_surface->width(), m_surface->height() }); }

private:
    void draw_1_bpp_bitmap(const Bitmap&, const Point&);
    void draw_32_bpp_bitmap(const Bitmap&, const Point&);
    void blit_32_bpp_bitmap(const Point& location, const Bitmap&, const Rect& source_rect);

private:
    Surface* m_surface;
    Rect     m_clip_rect;

    static constexpr size_t font_height = 16;
    static constexpr size_t font_width  = 8;
    static const u8         s_font[256][font_height];
};
}
