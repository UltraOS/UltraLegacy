#pragma once

#include "VideoDevice.h"
#include "VideoMode.h"

namespace kernel {

// Generic software rendering video device that operates on a liner framebuffer
// IMPORTANT: Works with BGR (RGB little endian)
class GenericVideoDevice : public VideoDevice {
public:
    GenericVideoDevice(const VideoMode&);

    StringView device_type() const override { return "Generic Video Device"_sv; }
    StringView device_model() const override { return "VESA BIOS"_sv; }

    const VideoMode& mode() const override { return m_mode; }

    Surface& surface() const override;

private:
    VideoMode m_mode;
    Surface* m_surface { nullptr };
};
}
