#pragma once

#include "Core/Boot.h"
#include "VideoDevice.h"

namespace kernel {

// Generic software rendering video device that operates on a liner framebuffer
class GenericVideoDevice : public VideoDevice {
public:
    GenericVideoDevice(const VideoMode&);

    StringView name() const override { return "Generic Video Device"; }

    void draw_bitmap(u8 *bitmap, size_t size, size_t x, size_t y) override;

private:
    VideoMode m_mode;
};
}
