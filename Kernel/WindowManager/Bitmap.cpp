#include "Bitmap.h"
#include "Core/Runtime.h"

namespace kernel {

Bitmap::Bitmap(void*  data,
               size_t width,
               size_t height,
               bool   owns_data,
               Format format,
               Color* palette,
               bool   owns_palette)
    : BitmapBase(width, height, format), m_data(reinterpret_cast<u8*>(data)), m_data_ownership(owns_data),
      m_palette(palette), m_palette_ownership(owns_palette)
{
}

BitmapView::BitmapView(const void* data, size_t width, size_t height, Format format, const Color* palette)
    : BitmapBase(width, height, format), m_data(reinterpret_cast<const u8*>(data)), m_palette(palette)
{
}

size_t BitmapBase::bpp() const
{
    switch (format()) {
    case Format::INDEXED_1_BPP: return 1;
    case Format::INDEXED_2_BPP: return 2;
    case Format::INDEXED_4_BPP: return 4;
    case Format::INDEXED_8_BPP: return 8;
    case Format::RGB_24_BPP: return 24;
    case Format::RGBA_32_BPP: return 32;
    }
}

void* Bitmap::scanline_at(size_t y)
{
    return Address(m_data).as_pointer<u8>() + pitch() * y;
}

const void* Bitmap::scanline_at(size_t y) const
{
    return Address(m_data).as_pointer<u8>() + pitch() * y;
}

const void* BitmapView::scanline_at(size_t y) const
{
    return Address(m_data).as_pointer<u8>() + pitch() * y;
}

Color& Bitmap::color_at(size_t index)
{
    ASSERT(palette() != nullptr);
    ASSERT(index < SET_BIT(bpp())); // 2^bpp == palette entry count

    return m_palette[index];
}

const Color& Bitmap::color_at(size_t index) const
{
    ASSERT(palette() != nullptr);
    ASSERT(index < SET_BIT(bpp())); // 2^bpp == palette entry count

    return m_palette[index];
}

const Color& BitmapView::color_at(size_t index) const
{
    ASSERT(palette() != nullptr);
    ASSERT(index < SET_BIT(bpp())); // 2^bpp == palette entry count

    return m_palette[index];
}

Bitmap::~Bitmap()
{
    if (m_data_ownership)
        delete[] m_data;
    if (m_palette_ownership)
        delete[] m_palette;
}
}
