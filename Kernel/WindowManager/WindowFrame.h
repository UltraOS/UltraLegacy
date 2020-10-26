#pragma once

#include "Bitmap.h"
#include "Color.h"
#include "Rect.h"

namespace kernel {

class Window;

class WindowFrame {
    MAKE_NONCOPYABLE(WindowFrame);

public:
    WindowFrame(Window& owner);
    void paint();
    Rect rect() const;
    Rect view_rect() const;

    Rect draggable_rect() const;
    Rect close_button_rect() const;
    Rect maximize_button_rect() const;
    Rect minimize_button_rect() const;

    void on_close_button_hovered();
    void on_close_button_released();

    void on_minimize_button_hovered();
    void on_minimize_button_released();

    void on_maximize_button_hovered();
    void on_maximize_button_released();

private:
    Rect raw_draggable_rect() const;

    Rect raw_close_button_rect() const;
    void draw_close_button(Color fill);

    Rect raw_maximize_button_rect() const;
    void draw_maximize_button(Color fill);

    Rect raw_minimize_button_rect() const;
    void draw_minimize_button(Color fill);

private:
    Window& m_owner;
};

}
