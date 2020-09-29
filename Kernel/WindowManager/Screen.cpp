#include "Screen.h"
#include "Drivers/Video/VideoDevice.h"
#include "WindowManager.h"

namespace kernel {

Screen* Screen::s_instance;

Screen::Screen(VideoDevice& device) : m_device(device), m_rect(0, 0, device.mode().width, device.mode().height)
{
    m_cursor.set_location(rect().center());
}

void Screen::check_if_focused_window_should_change()
{
    RefPtr<Window> window_that_should_be_focused;

    for (const auto& window: WindowManager::the().windows()) {
        if (window->rect().intersects(m_cursor.rect().translated(m_cursor.location()))) {
            // cannot break here because we're going from bottom to top
            // TODO: optimize?
            window_that_should_be_focused = window;
        }
    }

    // TODO: the amount of race conditions here is insane?
    if (window_that_should_be_focused) {
        window_that_should_be_focused->set_focused();
    } else {
        WindowManager::the().desktop()->set_focused();
    }
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
