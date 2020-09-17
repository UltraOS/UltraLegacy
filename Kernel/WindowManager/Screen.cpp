#include "Screen.h"
#include "Drivers/Video/VideoDevice.h"

namespace kernel {

Screen* Screen::s_instance;

Screen::Screen() : m_rect(0, 0, VideoDevice::the().mode().width, VideoDevice::the().mode().height)
{
    m_cursor.set_location(rect().center());
}

void Screen::recalculate_cursor_position(i16 delta_x, i16 delta_y)
{
    bool left_movement = delta_x < 0;
    bool down_movement = delta_y < 0;

    size_t new_x = 0;
    size_t new_y = 0;

    auto current_x = cursor().location().x();
    auto current_y = cursor().location().y();

    if (left_movement) {
        delta_x *= -1;

        if (current_x > static_cast<decltype(current_x)>(delta_x))
            new_x = current_x - delta_x;
    } else {
        if ((current_x + delta_x + m_cursor.bitmap().width()) < rect().width())
            new_x = current_x + delta_x;
        else
            new_x = rect().width() - m_cursor.bitmap().width();
    }

    if (down_movement) {
        delta_y *= -1;

        if ((current_y + delta_y + m_cursor.bitmap().height()) < rect().height())
            new_y = current_y + delta_y;
        else
            new_y = rect().height() - m_cursor.bitmap().height();
    } else {
        if (current_y > static_cast<decltype(current_x)>(delta_y))
            new_y = current_y - delta_y;
    }

    m_cursor.set_location({ new_x, new_y });
}
}
