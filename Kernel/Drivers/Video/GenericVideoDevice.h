#pragma once

#include "VideoDevice.h"
#include "VideoMode.h"

namespace kernel {

// Generic software rendering video device that operates on a liner framebuffer
// IMPORTANT: Works with BGR (RGB little endian)
class GenericVideoDevice : public VideoDevice {
    MAKE_SINGLETON_INHERITABLE(VideoDevice, GenericVideoDevice, const VideoMode&);

public:
    StringView name() const override { return "Generic Video Device"; }

    const VideoMode& mode() const override { return m_mode; }

    Surface& surface() const override;

private:
    VideoMode m_mode;
    Surface* m_surface { nullptr };
};
}
