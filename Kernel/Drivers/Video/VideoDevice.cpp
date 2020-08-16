#include "VideoDevice.h"
#include "GenericVideoDevice.h"

namespace kernel {

VideoDevice* VideoDevice::s_device;

void VideoDevice::discover_and_setup(const VideoMode& video_mode)
{
    // enumerate PCI for BXVGA/SVGA/OTHERS

    s_device = new GenericVideoDevice(video_mode);
}
}
