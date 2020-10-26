#pragma once

#include "Bitmap.h"
#include "Color.h"
#include "Common/RefPtr.h"

namespace kernel {

class Theme {
public:
    enum class Button {
        CLOSE,
        MAXIMIZE,
        MINIMIZE,
    };

    enum class ButtonState { PRESSED, HOVERED, RELEASED };

    static RefPtr<Theme> make_default();

    virtual Color desktop_background_color() const = 0;
    virtual Color window_frame_color() const       = 0;
    virtual Color taskbar_color() const            = 0;

    virtual size_t            width_for_button(Button) const                    = 0;
    virtual Color             color_for_button_state(Button, ButtonState) const = 0;
    virtual const BitmapView& button_bitmap(Button) const                       = 0;

    virtual const BitmapView& cursor_bitmap() const = 0;

    virtual size_t upper_window_frame_height() const = 0;

    virtual ~Theme() = default;
};
}
