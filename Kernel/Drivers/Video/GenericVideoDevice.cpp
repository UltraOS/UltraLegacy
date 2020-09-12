#include "GenericVideoDevice.h"
#include "Common/Logger.h"
#include "Common/Math.h"
#include "Memory/AddressSpace.h"

namespace kernel {

GenericVideoDevice::GenericVideoDevice(const VideoMode& video_mode) : m_mode(video_mode)
{
#ifdef ULTRA_32
    auto bytes_to_allocate = m_mode.pitch * m_mode.height;
    auto pages             = ceiling_divide(bytes_to_allocate, Page::size);

    log() << "GenericVideoDevice: allocating " << pages * Page::size << " bytes for the framebuffer...";

    auto range = AddressSpace::of_kernel().allocator().allocate_range(pages * Page::size);

    for (size_t i = 0; i < pages; ++i) {
        auto offset = i * Page::size;
        AddressSpace::of_kernel().map_supervisor_page(range.begin() + offset, m_mode.framebuffer + offset);
    }

    m_mode.framebuffer = range.begin();
#endif

    log() << "Initialized \"" << name() << "\": " << mode().width << "x" << mode().height << " @ " << mode().bpp
          << " bpp";
}
}
