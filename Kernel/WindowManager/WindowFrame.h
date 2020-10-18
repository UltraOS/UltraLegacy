#pragma once

#include "Color.h"
#include "Rect.h"

namespace kernel {

class Window;

class WindowFrame {
public:
    static constexpr size_t upper_frame_height  = 20;
    static constexpr Color  default_frame_color = { 0x8d, 0x26, 0x64 };

    WindowFrame(Window& owner);
    void paint();
    Rect rect() const;
    Rect view_rect() const;
    Rect translated_top_rect() const;

private:
    Window& m_owner;
};

}
