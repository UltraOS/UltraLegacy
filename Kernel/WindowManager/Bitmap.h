#pragma once

#include "Common/Types.h"
#include "Utilities.h"

namespace kernel {

class Bitmap {
public:
    enum class Format : u8 {
        INDEXED_1_BPP,
        INDEXED_2_BPP,
        INDEXED_4_BPP,
        INDEXED_8_BPP,
        RGB_24_BPP,
        RGBA_32_BPP,
    };

    Bitmap(size_t width, size_t height, Format format, size_t pitch = 0);

    size_t bpp() const;
    size_t pitch() const { return m_pitch; }
    Format format() const { return m_format; }
    size_t width() const { return m_width; }
    size_t height() const { return m_height; }
    bool   is_indexed() const;

    virtual const void*  scanline_at(size_t y) const  = 0;
    virtual const Color& color_at(size_t index) const = 0;
    virtual const Color* palette() const              = 0;

    virtual ~Bitmap() = default;

private:
    void calculate_pitch();

private:
    size_t m_width { 0 };
    size_t m_height { 0 };
    size_t m_pitch { 0 };

    Format m_format;
};

class MutableBitmap final : public Bitmap {
public:
    MutableBitmap(void* data, size_t width, size_t height, Format, Color* palette = nullptr, size_t pitch = 0);

    void*       scanline_at(size_t y);
    const void* scanline_at(size_t y) const override;

    Color&       color_at(size_t index);
    const Color& color_at(size_t index) const override;

    Color*       palette() { return m_palette; }
    const Color* palette() const override { return m_palette; }

private:
    u8*    m_data { nullptr };
    Color* m_palette { nullptr };
};

using Surface = MutableBitmap;

class BitmapView final : public Bitmap {
public:
    BitmapView(const void* data, size_t width, size_t height, Format, const Color* palette = nullptr, size_t pitch = 0);

    const void*  scanline_at(size_t y) const override;
    const Color& color_at(size_t index) const override;
    const Color* palette() const override { return m_palette; }

private:
    const u8*    m_data { nullptr };
    const Color* m_palette { nullptr };
};
}
