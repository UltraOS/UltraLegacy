#pragma once

#include "Core/Boot.h"

#include "Drivers/Device.h"
#include "WindowManager/Bitmap.h"

namespace kernel {

class VideoDevice : public Device {
public:
    Type type() const override { return Type::VIDEO; }

    virtual const VideoMode& mode() const = 0;

    static void discover_and_setup(const VideoMode&);

    virtual Surface& surface() const = 0;

    static VideoDevice& the()
    {
        ASSERT(s_device != nullptr);

        return *s_device;
    }

private:
    static VideoDevice* s_device;
};
}
