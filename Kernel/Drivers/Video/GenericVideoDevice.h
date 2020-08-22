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

    void draw_at(size_t x, size_t y, RGBA pixel) override;

    Address framebuffer() { return m_mode.framebuffer; }
    size_t  bpp() const override { return m_mode.pitch; }
    size_t  pitch() const override { return m_mode.pitch; }
    size_t  width() const override { return m_mode.width; }
    size_t  height() const override { return m_mode.height; }

private:
    VideoMode m_mode;
};
}
