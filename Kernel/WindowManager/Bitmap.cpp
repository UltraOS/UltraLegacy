#include "Bitmap.h"
#include "Common/Math.h"
#include "Core/Runtime.h"

namespace kernel {

Bitmap::Bitmap(ssize_t width, ssize_t height, Format format, ssize_t pitch)
    : m_width(width)
    , m_height(height)
    , m_pitch(pitch)
    , m_format(format)
{
    if (m_pitch == 0)
        calculate_pitch();
}

void Bitmap::calculate_pitch()
{
    m_pitch = ceiling_divide(m_width * bpp(), static_cast<decltype(m_width)>(8));
}

MutableBitmap::MutableBitmap(void* data, ssize_t width, ssize_t height, Format format, Color* palette, ssize_t pitch)
    : Bitmap(width, height, format, pitch)
    , m_data(reinterpret_cast<u8*>(data))
    , m_palette(palette)
{
}

BitmapView::BitmapView(const void* data, ssize_t width, ssize_t height, Format format, const Color* palette, ssize_t pitch)
    : Bitmap(width, height, format, pitch)
    , m_data(reinterpret_cast<const u8*>(data))
    , m_palette(palette)
{
}

bool Bitmap::is_indexed() const
{
    switch (format()) {
    case Format::INDEXED_1_BPP:
    case Format::INDEXED_2_BPP:
    case Format::INDEXED_4_BPP:
    case Format::INDEXED_8_BPP:
        return true;
    default:
        return false;
    }
}

ssize_t Bitmap::bpp() const
{
    switch (format()) {
    case Format::INDEXED_1_BPP:
        return 1;
    case Format::INDEXED_2_BPP:
        return 2;
    case Format::INDEXED_4_BPP:
        return 4;
    case Format::INDEXED_8_BPP:
        return 8;
    case Format::RGB_24_BPP:
        return 24;
    case Format::RGBA_32_BPP:
        return 32;
    default:
        ASSERT(false && "Unknown bitmap type");
        return 0;
    }
}

void* MutableBitmap::scanline_at(ssize_t y)
{
    return Address(m_data).as_pointer<u8>() + pitch() * y;
}

const void* MutableBitmap::scanline_at(ssize_t y) const
{
    return Address(m_data).as_pointer<u8>() + pitch() * y;
}

const void* BitmapView::scanline_at(ssize_t y) const
{
    return Address(m_data).as_pointer<u8>() + pitch() * y;
}

Color& MutableBitmap::color_at(size_t index)
{
    ASSERT(palette() != nullptr);
    ASSERT(index < SET_BIT(bpp())); // 2^bpp == palette entry count

    return m_palette[index];
}

const Color& MutableBitmap::color_at(size_t index) const
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
}
