#pragma once

#include "Bitmap.h"
#include "Color.h"
#include "Rect.h"

namespace kernel {

class Window;

class WindowFrame {
    MAKE_NONCOPYABLE(WindowFrame);

public:
    enum class Button {
        CLOSE,
        MAXIMIZE,
        MINIMIZE,
    };

    enum class ButtonState {
        PRESSED,
        HOVERED,
        RELEASED
    };

    enum class Frame {
        TOP,
        LEFT,
        RIGHT,
        BOTTOM
    };

    WindowFrame(Window& owner);
    void paint();
    Rect rect() const;
    Rect view_rect() const;

    Rect draggable_rect() const;
    Rect rect_for_button(Button) const;

    void on_button_state_changed(Button, ButtonState);

private:
    Rect raw_draggable_rect() const;
    Rect raw_rect_for_button(Button) const;
    Rect raw_rect_for_frame(Frame) const;

    void draw_button(Button, ButtonState);
    void draw_title();

private:
    Window& m_owner;
};

}
