#include "Painter.h"
#include "Drivers/Video/VideoDevice.h"

namespace kernel {

Painter::Painter(Surface* surface)
    : m_surface(surface)
{
    ASSERT(surface != nullptr);
    ASSERT(surface->bpp() == 32 && "Painter can only operate on 32bpp surfaces atm");
    reset_clip_rect();
}

void Painter::fill_rect(const Rect& rect, Color color)
{
    auto drawable_rect = rect.intersected(m_clip_rect);

    if (drawable_rect.empty())
        return;

    auto bytes_per_pixel = m_surface->bpp() / 8;
    auto x_offset = drawable_rect.top_left().x() * bytes_per_pixel;
    Address pixels_begin = Address(m_surface->scanline_at(drawable_rect.top_left().y())) + x_offset;
    auto step = m_surface->pitch() - (x_offset + (drawable_rect.width() * bytes_per_pixel)) + x_offset;

    for (auto y = 0; y < drawable_rect.height(); ++y) {
        for (auto x = 0; x < drawable_rect.width(); ++x) {
            *Address(pixels_begin).as_pointer<u32>() = color.as_u32();
            pixels_begin += bytes_per_pixel;
        }
        pixels_begin += step;
    }
}

void Painter::draw_at(size_t x, size_t y, Color pixel)
{
    if (!pixel.a())
        return;

    Address pixels_begin = Address(m_surface->scanline_at(y)) + (x * m_surface->bpp() / 8);
    *pixels_begin.as_pointer<u32>() = pixel.as_u32();
}

void Painter::draw_bitmap(const Bitmap& bitmap, const Point& point)
{
    switch (bitmap.format()) {
    case Bitmap::Format::INDEXED_1_BPP:
        draw_1_bpp_bitmap(bitmap, point);
        break;
    case Bitmap::Format::RGBA_32_BPP:
        draw_32_bpp_bitmap(bitmap, point);
        break;
    default:
        warning() << "Painter: cannot draw unknown format " << static_cast<u8>(bitmap.format());
    }
}

void Painter::blit(const Point& location, const Bitmap& bitmap, const Rect& source_rect)
{
    switch (bitmap.format()) {
    case Bitmap::Format::INDEXED_1_BPP:
        blit_1_bpp_bitmap(location, bitmap, source_rect);
        break;
    case Bitmap::Format::RGBA_32_BPP:
        blit_32_bpp_bitmap(location, bitmap, source_rect);
        break;
    default:
        warning() << "Painter: cannot draw unknown format " << static_cast<u8>(bitmap.format());
    }
}

void Painter::blit_1_bpp_bitmap(const Point& location, const Bitmap& bitmap, const Rect& source_rect)
{
    Rect original_rect(location, source_rect.width(), source_rect.height());
    Rect drawable_rect = original_rect.intersected(m_clip_rect);

    if (drawable_rect.empty())
        return;

    auto y_begin = drawable_rect.top() - original_rect.top();
    auto y_end = drawable_rect.bottom() - original_rect.top();
    auto x_begin = drawable_rect.left() - original_rect.left();
    auto x_end = drawable_rect.right() - original_rect.left();

    const u8* source = Address(bitmap.scanline_at(source_rect.top() + y_begin)).as_pointer<u8>();
    auto x_source_begin = source_rect.left() + x_begin;
    auto source_byte_begin = x_source_begin / 8;
    source += source_byte_begin;
    size_t source_bit_begin = x_source_begin - (source_byte_begin * 8);

    u32* destination = Address(m_surface->scanline_at(drawable_rect.top())).as_pointer<u32>() + drawable_rect.left();
    auto destination_skip = m_surface->pitch() / sizeof(u32);

    auto source_byte = source_byte_begin;
    auto source_bit = source_bit_begin;

    for (auto y = y_begin; y <= y_end; ++y) {
        for (auto x = 0; x <= (x_end - x_begin); ++x) {
            if (source_bit == 8) {
                source_bit = 0;
                source_byte++;
            }

            auto color = bitmap.color_at(!!(source[source_byte] & SET_BIT(source_bit++)));
            if (!color.a()) // fully transparent
                continue;

            destination[x] = color.as_u32();
        }
        source += bitmap.pitch();
        source_byte = source_byte_begin;
        source_bit = source_bit_begin;

        destination += destination_skip;
    }
}

void Painter::blit_32_bpp_bitmap(const Point& location, const Bitmap& bitmap, const Rect& source_rect)
{
    Rect original_rect(location, source_rect.width(), source_rect.height());
    Rect drawable_rect = original_rect.intersected(m_clip_rect);

    if (drawable_rect.empty())
        return;

    auto y_begin = drawable_rect.top() - original_rect.top();
    auto y_end = drawable_rect.bottom() - original_rect.top();
    auto x_begin = drawable_rect.left() - original_rect.left();
    auto x_end = drawable_rect.right() - original_rect.left();

    const u32* source
        = Address(bitmap.scanline_at(source_rect.top() + y_begin)).as_pointer<u32>() + source_rect.left() + x_begin;
    auto source_skip = bitmap.pitch() / sizeof(u32);

    u32* destination = Address(m_surface->scanline_at(drawable_rect.top())).as_pointer<u32>() + drawable_rect.left();
    auto destination_skip = m_surface->pitch() / sizeof(u32);

    for (auto y = y_begin; y <= y_end; ++y) {
        for (auto x = 0; x <= (x_end - x_begin); ++x) {
            destination[x] = source[x];
        }
        source += source_skip;
        destination += destination_skip;
    }
}

void Painter::draw_1_bpp_bitmap(const Bitmap& bitmap, const Point& point)
{
    size_t bytes_per_pixel = m_surface->bpp() / 8;
    Address pixels_begin = Address(m_surface->scanline_at(point.y())) + point.x() * bytes_per_pixel;

    for (auto y = 0; y < bitmap.height(); ++y) {
        auto* scanline = reinterpret_cast<const u8*>(bitmap.scanline_at(y));

        for (auto x = 0; x < bitmap.width(); ++x) {
            size_t byte = x / 8;
            size_t bit = SET_BIT(x - byte * 8);

            auto color = bitmap.color_at(!!(scanline[byte] & bit));
            if (!color.a()) // fully transparent
                continue;

            *Address(pixels_begin + (bytes_per_pixel * x)).as_pointer<u32>() = color.as_u32();
        }
        pixels_begin += m_surface->pitch();
    }
}

void Painter::draw_32_bpp_bitmap(const Bitmap& bitmap, const Point& point)
{
    size_t bytes_per_pixel = m_surface->bpp() / 8;
    Address pixels_begin = Address(m_surface->scanline_at(point.y())) + point.x() * bytes_per_pixel;

    for (auto y = 0; y < bitmap.height(); ++y) {
        auto* scanline = reinterpret_cast<const u8*>(bitmap.scanline_at(y));

        for (auto x = 0; x < bitmap.width(); ++x) {
            // TODO: blending

            *Address(pixels_begin + (bytes_per_pixel * x)).as_pointer<u32>()
                = *(Address(scanline).as_pointer<u32>() + x);
        }
        pixels_begin += m_surface->pitch();
    }
}

void Painter::draw_char(Point top_left, char c, Color char_color, Color fill_color)
{
    ASSERT(c >= 0);

    Color palette[2] { fill_color, char_color };

    BitmapView char_map(
        s_font[static_cast<size_t>(c)],
        font_width,
        font_height,
        Bitmap::Format::INDEXED_1_BPP,
        palette);

    blit_1_bpp_bitmap(top_left, char_map, { 0, 0, font_width, font_height });
}

// Fully handcrafted font, took a ton of effort :)
const u8 Painter::s_font[256][16] = {
    { 0 }, // Null (0)
    { 0 }, // Start of Heading (1)
    { 0 }, // Start of Text (2)
    { 0 }, // End of Text (3)
    { 0 }, // End of Transmission (4)
    { 0 }, // Enquiry (5)
    { 0 }, // Acknowledgement (6)
    { 0 }, // Bell (7)
    { 0 }, // Back Space (8)
    { 0 }, // Horizontal Tab (9)
    { 0 }, // Line Feed (10)
    { 0 }, // Vertical Tab (11)
    { 0 }, // Form Feed (12)
    { 0 }, // Carriage Return (13)
    { 0 }, // Shift Out / X-On (14)
    { 0 }, // Shift In / X-Off (15)
    { 0 }, // Data Line Escape (16)
    { 0 }, // Device Control 1 (17)
    { 0 }, // Device Control 2 (18)
    { 0 }, // Device Control 3 (19)
    { 0 }, // Device Control 4 (20)
    { 0 }, // Negative Acknowledgement (21)
    { 0 }, // Synchronous Idle (22)
    { 0 }, // End of Transmit Block (23)
    { 0 }, // Cancel (24)
    { 0 }, // End of Medium (25)
    { 0 }, // Substitute (26)
    { 0 }, // Escape (27)
    { 0 }, // File Separator (28)
    { 0 }, // Group Separator (29)
    { 0 }, // Record Separator (30)
    { 0 }, // Unit Separator (31)

    // ---- End of control characters ----

    // clang-format off
    { 0 }, // Space (32)
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00000000,
      0b00000000,
      0b00001000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00010010,
      0b00010010,
      0b00010010,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00010100,
      0b00010100,
      0b01111111,
      0b00010100,
      0b01111111,
      0b00010100,
      0b00010100,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00001000,
      0b00111110,
      0b01001001,
      0b00001001,
      0b00001001,
      0b00111110,
      0b01001000,
      0b01001000,
      0b01001001,
      0b00111110,
      0b00001000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00100010,
      0b00010101,
      0b00001010,
      0b00001000,
      0b00010100,
      0b00101010,
      0b00010001,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00011100,
      0b00100010,
      0b00100010,
      0b00011100,
      0b01001010,
      0b01010001,
      0b00100001,
      0b01011110,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00010000,
      0b00001000,
      0b00000100,
      0b00000100,
      0b00000100,
      0b00000100,
      0b00000100,
      0b00000100,
      0b00000100,
      0b00001000,
      0b00010000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000100,
      0b00001000,
      0b00010000,
      0b00010000,
      0b00010000,
      0b00010000,
      0b00010000,
      0b00010000,
      0b00010000,
      0b00001000,
      0b00000100,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00101010,
      0b00011100,
      0b00011100,
      0b00100010,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b01111111,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00001000,
      0b00001000,
      0b00000100,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01111111,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00011000,
      0b00011000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01000000,
      0b00100000,
      0b00010000,
      0b00001000,
      0b00000100,
      0b00000010,
      0b00000001,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00011110,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00011110,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00001100,
      0b00001010,
      0b00001001,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00111111,
      0b01000000,
      0b01000000,
      0b01000000,
      0b00111110,
      0b00000001,
      0b00000001,
      0b00000001,
      0b01111111,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00111111,
      0b01000000,
      0b01000000,
      0b01000000,
      0b00111111,
      0b01000000,
      0b01000000,
      0b01000000,
      0b00111111,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00110000,
      0b00101000,
      0b00100100,
      0b00100010,
      0b00100001,
      0b01111111,
      0b00100000,
      0b00100000,
      0b00100000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01111111,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00111111,
      0b01000000,
      0b01000000,
      0b01000000,
      0b00111111,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00111110,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00111111,
      0b01000001,
      0b01000001,
      0b01000001,
      0b00111110,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01111111,
      0b01000000,
      0b00100000,
      0b00010000,
      0b00001000,
      0b00000100,
      0b00000010,
      0b00000010,
      0b00000010,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00111110,
      0b01000001,
      0b01000001,
      0b01000001,
      0b00111110,
      0b01000001,
      0b01000001,
      0b01000001,
      0b00111110,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00111110,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01111110,
      0b01000000,
      0b01000000,
      0b01000000,
      0b00111111,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00011000,
      0b00011000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00011000,
      0b00011000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00011000,
      0b00011000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00010000,
      0b00001000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00010000,
      0b00001000,
      0b00000100,
      0b00000010,
      0b00000001,
      0b00000010,
      0b00000100,
      0b00001000,
      0b00010000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01111111,
      0b00000000,
      0b00000000,
      0b01111111,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000001,
      0b00000010,
      0b00000100,
      0b00001000,
      0b00010000,
      0b00001000,
      0b00000100,
      0b00000010,
      0b00000001,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00111110,
      0b01000001,
      0b01000000,
      0b00100000,
      0b00011000,
      0b00001000,
      0b00000000,
      0b00001000,
      0b00001000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00111110,
      0b01000001,
      0b01011001,
      0b01100101,
      0b01011001,
      0b00000001,
      0b00000001,
      0b01000001,
      0b00111110,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00011100,
      0b00100010,
      0b01000001,
      0b01000001,
      0b01111111,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01000001,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00111111,
      0b01000001,
      0b01000001,
      0b00111111,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01000001,
      0b00111111,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00011110,
      0b00100001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00100001,
      0b00011110,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00011111,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00011111,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01111111,
      0b00000001,
      0b00000001,
      0b00000001,
      0b01111111,
      0b00000001,
      0b00000001,
      0b00000001,
      0b01111111,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01111111,
      0b00000001,
      0b00000001,
      0b00000001,
      0b01111111,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01111110,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00111001,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01111110,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01111111,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01000001,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00111110,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00111110,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00010000,
      0b00010000,
      0b00010000,
      0b00010000,
      0b00010000,
      0b00010000,
      0b00010000,
      0b00010001,
      0b00001110,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00100001,
      0b00010001,
      0b00001001,
      0b00000101,
      0b00000011,
      0b00000101,
      0b00001001,
      0b00010001,
      0b00100001,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00011111,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01100011,
      0b01100011,
      0b01100011,
      0b01100011,
      0b01010101,
      0b01010101,
      0b01011101,
      0b01001001,
      0b01001001,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01000001,
      0b01000001,
      0b01000011,
      0b01000101,
      0b01001001,
      0b01010001,
      0b01100001,
      0b01000001,
      0b01000001,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00111110,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01000001,
      0b00111110,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00011111,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00011111,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00011110,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00111110,
      0b01100000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00011111,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00011111,
      0b00010001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00111110,
      0b01000001,
      0b00000001,
      0b00000001,
      0b00111110,
      0b01000000,
      0b01000000,
      0b01000001,
      0b00111110,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01111111,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01000001,
      0b00111110,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01000001,
      0b01000001,
      0b00100010,
      0b00010100,
      0b00001000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01001001,
      0b01001001,
      0b01001001,
      0b01001001,
      0b01001001,
      0b01011101,
      0b00100010,
      0b00100010,
      0b00100010,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01000001,
      0b00100010,
      0b00010100,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00010100,
      0b00100010,
      0b01000001,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01000001,
      0b00100010,
      0b00010100,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01111111,
      0b01000000,
      0b00100000,
      0b00010000,
      0b00001000,
      0b00000100,
      0b00000010,
      0b00000001,
      0b01111111,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00001111,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00001111,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000001,
      0b00000010,
      0b00000100,
      0b00001000,
      0b00010000,
      0b00100000,
      0b01000000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00001111,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001111,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00001000,
      0b00010100,
      0b00100010,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01111111,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000100,
      0b00001000,
      0b00010000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00111110,
      0b01000001,
      0b01000000,
      0b01111110,
      0b01000001,
      0b01100001,
      0b01011110,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000001,
      0b00000001,
      0b00011111,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00011111,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00011110,
      0b00100001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00100001,
      0b00011110,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01000000,
      0b01000000,
      0b01111100,
      0b01000010,
      0b01000010,
      0b01000010,
      0b01000010,
      0b01000010,
      0b01111100,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00011110,
      0b00100001,
      0b00100001,
      0b00111111,
      0b00000001,
      0b00100001,
      0b00011110,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00111100,
      0b00000010,
      0b00000010,
      0b00111111,
      0b00000010,
      0b00000010,
      0b00000010,
      0b00000010,
      0b00000010,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00101110,
      0b00110001,
      0b00100001,
      0b00100001,
      0b00110001,
      0b00101110,
      0b00100000,
      0b00010000,
      0b00001111,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000001,
      0b00000001,
      0b00011101,
      0b00100011,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00001000,
      0b00000000,
      0b00000000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00001000,
      0b00000000,
      0b00000000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001001,
      0b00000110,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000001,
      0b00000001,
      0b00010001,
      0b00001001,
      0b00000101,
      0b00000011,
      0b00000101,
      0b00001001,
      0b00010001,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00110110,
      0b01001001,
      0b01001001,
      0b01001001,
      0b01001001,
      0b01001001,
      0b01001001,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00011101,
      0b00100011,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    { 0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00011110,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00011110,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00011111,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00011111,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00011110,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00111110,
      0b00100000,
      0b00100000,
      0b00100000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00111101,
      0b00000011,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000001,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00111110,
      0b00000001,
      0b00000001,
      0b00011110,
      0b00100000,
      0b00100000,
      0b00011111,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000010,
      0b00000010,
      0b00011111,
      0b00000010,
      0b00000010,
      0b00000010,
      0b00000010,
      0b00000010,
      0b00111100,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00011110,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00010010,
      0b00001100,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01001001,
      0b01001001,
      0b01001001,
      0b01001001,
      0b01001001,
      0b01011101,
      0b00100010,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00100001,
      0b00010010,
      0b00001100,
      0b00001100,
      0b00001100,
      0b00010010,
      0b00100001,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00100001,
      0b00100001,
      0b00100001,
      0b00010010,
      0b00010010,
      0b00001100,
      0b00001000,
      0b00001000,
      0b00000110,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00111111,
      0b00100000,
      0b00010000,
      0b00001000,
      0b00000100,
      0b00000010,
      0b00111111,
      0b00000000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00110000,
      0b00001000,
      0b00000100,
      0b00000100,
      0b00000100,
      0b00000011,
      0b00000100,
      0b00000100,
      0b00000100,
      0b00001000,
      0b00110000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00001000,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000110,
      0b00001000,
      0b00010000,
      0b00010000,
      0b00010000,
      0b01100000,
      0b00010000,
      0b00010000,
      0b00010000,
      0b00001000,
      0b00000110,
      0b00000000,
      0b00000000,
    },
    {
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b01001110,
      0b00110001,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
      0b00000000,
    }
};
// clang-format on
}
