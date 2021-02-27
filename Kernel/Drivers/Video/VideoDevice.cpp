#include "VideoDevice.h"
#include "Drivers/DeviceManager.h"
#include "GenericVideoDevice.h"

namespace kernel {

VideoDevice& VideoDevice::primary()
{
    auto* device = DeviceManager::the().primary_of(Category::VIDEO);
    ASSERT(device != nullptr);

    return *static_cast<VideoDevice*>(device);
}

bool VideoDevice::is_ready()
{
    return DeviceManager::is_initialized() && DeviceManager::the().primary_of(Category::VIDEO) != nullptr;
}

void VideoDevice::discover_and_setup(LoaderContext* context)
{
    // enumerate PCI for BXVGA/SVGA/OTHERS

    ASSERT(context->type == LoaderContext::Type::BIOS);

    auto* bios_context = static_cast<BIOSContext*>(context);

    new GenericVideoDevice(static_cast<VideoMode&>(bios_context->video_mode));
}
}
