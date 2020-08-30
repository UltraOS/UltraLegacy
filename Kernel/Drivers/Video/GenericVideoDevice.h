#pragma once

#include "Core/Boot.h"
#include "VideoDevice.h"

namespace kernel {

// Generic software rendering video device that operates on a liner framebuffer
// IMPORTANT: Works with BGR (RGB little endian)
class GenericVideoDevice : public VideoDevice {
public:
    GenericVideoDevice(const VideoMode&);

    StringView name() const override { return "Generic Video Device"; }

    void fill_rect(const Rect&, Color color) override;
    void draw_at(size_t x, size_t y, Color pixel) override;
    void draw_char(Point top_left, char, Color char_color, Color fill_color) override;

    Address framebuffer() override { return m_mode.framebuffer; }
    size_t  bpp() const override { return m_mode.pitch; }
    size_t  pitch() const override { return m_mode.pitch; }
    size_t  width() const override { return m_mode.width; }
    size_t  height() const override { return m_mode.height; }

private:
    VideoMode m_mode;

    static constexpr size_t font_height = 16;
    static constexpr size_t font_width  = 8;
    static const u8         s_font[256][font_height];
};
}
