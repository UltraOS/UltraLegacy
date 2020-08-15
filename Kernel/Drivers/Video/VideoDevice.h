#pragma once

#include "Drivers/Device.h"

namespace kernel {

class VideoDevice : public Device {
public:
    Type type() const override { return Type::VIDEO; }

    // void fill_rect();
    // void blit();
    virtual void draw_bitmap(u8* bitmap, size_t size, size_t x, size_t y) = 0;
};
}
