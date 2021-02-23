#pragma once

#include "Core/Boot.h"
#include "Drivers/Device.h"
#include "VideoMode.h"
#include "WindowManager/Bitmap.h"

namespace kernel {

class VideoDevice : public Device {
    MAKE_INHERITABLE_SINGLETON(VideoDevice) = default;

public:
    Type device_type() const override { return Type::VIDEO; }

    virtual const VideoMode& mode() const = 0;

    static void discover_and_setup(LoaderContext*);

    virtual Surface& surface() const = 0;

    static VideoDevice& the()
    {
        ASSERT(s_device != nullptr);

        return *s_device;
    }

    static bool is_ready() { return s_device != nullptr; }

private:
    static VideoDevice* s_device;
};
}
