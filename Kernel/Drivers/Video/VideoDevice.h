#pragma once

#include "Core/Boot.h"

#include "Drivers/Device.h"

#include "Utilities.h"

namespace kernel {

class VideoDevice : public Device {
public:
    Type type() const override { return Type::VIDEO; }

    // void blit();
    // draw_bitmap()

    virtual void fill_rect(const Rect&, Color color)                                 = 0;
    virtual void draw_at(size_t x, size_t y, Color pixel)                            = 0;
    virtual void draw_char(Point top_left, char, Color char_color, Color fill_color) = 0;

    virtual Address framebuffer()  = 0;
    virtual size_t  width() const  = 0;
    virtual size_t  height() const = 0;
    virtual size_t  bpp() const    = 0;
    virtual size_t  pitch() const  = 0;

    static void discover_and_setup(const VideoMode&);

    static VideoDevice& the()
    {
        ASSERT(s_device != nullptr);

        return *s_device;
    }

private:
    static VideoDevice* s_device;
};
}
