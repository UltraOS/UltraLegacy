#include "GenericVideoDevice.h"
#include "Common/Logger.h"
#include "Common/Math.h"
#include "Memory/AddressSpace.h"
#include "Memory/PAT.h"

namespace kernel {

GenericVideoDevice::GenericVideoDevice(const VideoMode& video_mode)
    : m_mode(video_mode)
{
    static constexpr u8 wc_pat_index = 4;

    // This makes writing to video memory on real hardware like 10 times faster
    PAT::the().set_entry(wc_pat_index, PAT::MemoryType::WRITE_COMBINING);

#ifdef ULTRA_32
    auto bytes_to_allocate = m_mode.pitch * m_mode.height;
    auto pages = ceiling_divide(bytes_to_allocate, Page::size);

    auto aligned_bytes_to_allocate = pages * Page::size;

    log() << "GenericVideoDevice: allocating " << aligned_bytes_to_allocate << " bytes for the framebuffer...";

    auto range = AddressSpace::of_kernel().allocator().allocate_range(aligned_bytes_to_allocate);

    size_t page_index = 0;
    for (Address current_address = range.begin(); current_address < range.end(); current_address += Page::size) {
        auto offset = page_index++ * Page::size;
        AddressSpace::of_kernel().map_supervisor_page(current_address, m_mode.framebuffer + offset);
        auto indices = AddressSpace::virtual_address_as_paging_indices(current_address);

        AddressSpace::of_kernel().pt_at(indices.first()).entry_at(indices.second()).set_pat_index(wc_pat_index, true);
        AddressSpace::of_kernel().flush_at(current_address);
    }

    m_mode.framebuffer = range.begin();

#else
    // TODO: align manually
    ASSERT_HUGE_PAGE_ALIGNED(m_mode.framebuffer);

    auto pages = ceiling_divide(static_cast<size_t>(m_mode.pitch * m_mode.height), Page::huge_size);
    auto framebuffer_end = m_mode.framebuffer + pages * Page::huge_size;

    for (Address current_address = m_mode.framebuffer; current_address < framebuffer_end;
         current_address += Page::huge_size) {
        auto indices = AddressSpace::virtual_address_as_paging_indices(current_address);
        AddressSpace::of_kernel()
            .pdpt_at(indices.first())
            .pdt_at(indices.second())
            .entry_at(indices.third())
            .set_pat_index(wc_pat_index, false);
        AddressSpace::of_kernel().flush_at(current_address);
    }
#endif

    m_surface = new Surface(
        Address(m_mode.framebuffer).as_pointer<u8>(),
        m_mode.width,
        m_mode.height,
        m_mode.bpp == 24 ? Surface::Format::RGB_24_BPP : Surface::Format::RGBA_32_BPP,
        nullptr,
        m_mode.pitch);

    log() << "Initialized \"" << name() << "\": " << mode().width << "x" << mode().height << " @ " << mode().bpp
          << " bpp";
}

Surface& GenericVideoDevice::surface() const
{
    return *m_surface;
}
}
