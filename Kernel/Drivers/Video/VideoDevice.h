#pragma once

#include "Core/Boot.h"
#include "Drivers/Device.h"
#include "VideoMode.h"
#include "WindowManager/Bitmap.h"

namespace kernel {

class VideoDevice : public Device {
public:
    VideoDevice()
        : Device(Category::VIDEO)
    {
    }

    virtual const VideoMode& mode() const = 0;

    static void discover_and_setup(LoaderContext*);

    virtual Surface& surface() const = 0;

    static VideoDevice& primary();
    static bool is_ready();
};
}
