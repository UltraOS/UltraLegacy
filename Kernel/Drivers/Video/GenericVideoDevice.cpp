#include "GenericVideoDevice.h"

namespace kernel {

GenericVideoDevice::GenericVideoDevice(const VideoMode& video_mode)
    : m_mode(video_mode)
{
}

void GenericVideoDevice::draw_bitmap(u8* bitmap, size_t size, size_t x, size_t y)
{
    (void) bitmap;
    (void) size;
    (void) x;
    (void) y;
}
}
