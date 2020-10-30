#pragma once

#include "Bitmap.h"
#include "Color.h"
#include "Common/RefPtr.h"
#include "WindowFrame.h"

namespace kernel {

class Theme {
public:
    static RefPtr<Theme> make_default();

    virtual Color desktop_background_color() const = 0;
    virtual Color window_frame_color() const = 0;
    virtual Color taskbar_color() const = 0;

    virtual ssize_t width_for_button(WindowFrame::Button) const = 0;
    virtual Color color_for_button_state(WindowFrame::Button, WindowFrame::ButtonState) const = 0;
    virtual const BitmapView& button_bitmap(WindowFrame::Button) const = 0;

    virtual const BitmapView& cursor_bitmap() const = 0;

    virtual ssize_t upper_window_frame_height() const = 0;
    virtual ssize_t side_window_frame_width() const = 0;
    virtual ssize_t bottom_width_frame_height() const = 0;

    virtual ~Theme() = default;
};
}
