#pragma once

#include "Common/Types.h"
#include "Utilities.h"

namespace kernel {

class BitmapBase {
public:
    enum class Format {
        INDEXED_1_BPP,
        INDEXED_2_BPP,
        INDEXED_4_BPP,
        INDEXED_8_BPP,
        RGB_24_BPP,
        RGBA_32_BPP,
    };

    BitmapBase(size_t width, size_t height, Format);

    size_t bpp() const;
    size_t pitch() const { return bpp() * m_width; }
    Format format() const { return m_format; }

    virtual const void*  scanline_at(size_t y) const  = 0;
    virtual const Color& color_at(size_t index) const = 0;
    virtual const Color* palette() const              = 0;

    virtual ~BitmapBase() = 0;

private:
    size_t m_width { 0 };
    size_t m_height { 0 };

    Format m_format;
};

class Bitmap final : public BitmapBase {
public:
    Bitmap(void*  data,
           size_t width,
           size_t height,
           bool   owns_data,
           Format,
           Color* palette      = nullptr,
           bool   owns_palette = false);

    void*       scanline_at(size_t y);
    const void* scanline_at(size_t y) const override;

    Color&       color_at(size_t index);
    const Color& color_at(size_t index) const override;

    Color*       palette() { return m_palette; }
    const Color* palette() const override { return m_palette; }

    ~Bitmap();

private:
    u8*  m_data { nullptr };
    bool m_data_ownership { false };

    Color* m_palette { nullptr };
    bool   m_palette_ownership { false };
};

class BitmapView final : public BitmapBase {
public:
    BitmapView(const void* data, size_t width, size_t height, Format, const Color* palette = nullptr);

    const void*  scanline_at(size_t y) const override;
    const Color& color_at(size_t index) const override;
    const Color* palette() const override { return m_palette; }

private:
    const u8*    m_data { nullptr };
    const Color* m_palette { nullptr };
};
}
