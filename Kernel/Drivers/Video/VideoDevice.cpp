#include "VideoDevice.h"
#include "GenericVideoDevice.h"

namespace kernel {

VideoDevice* VideoDevice::s_device;

void VideoDevice::discover_and_setup(LoaderContext* context)
{
    // enumerate PCI for BXVGA/SVGA/OTHERS

    ASSERT(context->type == LoaderContext::Type::BIOS);

    auto* bios_context = static_cast<BIOSContext*>(context);

    s_device = new GenericVideoDevice(static_cast<VideoMode&>(bios_context->video_mode));
}
}
