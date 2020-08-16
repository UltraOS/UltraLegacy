#include "Common/Logger.h"
#include "Common/Math.h"
#include "GenericVideoDevice.h"
#include "Memory/AddressSpace.h"

namespace kernel {

GenericVideoDevice::GenericVideoDevice(const VideoMode& video_mode)
    : m_mode(video_mode)
{
    #ifdef ULTRA_32
    auto bytes_to_allocate = m_mode.pitch * m_mode.height;
    auto pages = ceiling_divide(bytes_to_allocate, Page::size);

    log() << "GenericVideoDevice: allocating " << pages * Page::size << " bytes for the framebuffer...";

    auto range = AddressSpace::of_kernel().allocator().allocate_range(pages * Page::size);

    for (size_t i = 0; i < pages; ++i) {
        auto offset = i * Page::size;
        AddressSpace::of_kernel().map_supervisor_page(range.begin() + offset, m_mode.framebuffer + offset);
    }

    m_mode.framebuffer = range.begin();
    #endif

    log() << "Initialized \"" << name() << "\": " << width()
          << "x" << height() << " @ " << m_mode.bpp << " bpp";
}

void GenericVideoDevice::draw_at(size_t x, size_t y, RGBA pixel)
{
    size_t offset = (m_mode.pitch * y) + (x * m_mode.bpp / 8);

    if (m_mode.bpp == 32)
        *Address(m_mode.framebuffer + offset).as_pointer<u32>() = pixel.as_u32();
    else {
        ASSERT(m_mode.bpp == 24);
        *Address(m_mode.framebuffer + offset + 0).as_pointer<u8>() = pixel.b;
        *Address(m_mode.framebuffer + offset + 1).as_pointer<u8>() = pixel.g;
        *Address(m_mode.framebuffer + offset + 2).as_pointer<u8>() = pixel.r;
    }
}
}
