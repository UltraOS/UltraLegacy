#include "Screen.h"
#include "Drivers/Video/VideoDevice.h"
#include "WindowManager.h"

namespace kernel {

Screen* Screen::s_instance;

Screen::Screen(VideoDevice& device)
    : m_device(device)
    , m_rect(0, 0, device.mode().width, device.mode().height)
{
    auto cursor_location = rect().center();
    m_cursor.set_location(cursor_location);
    m_shadow_cursor_location = cursor_location;
}

void Screen::check_if_focused_window_should_change()
{
    auto& window_list = WindowManager::the().windows();
    auto window_that_should_be_focused = window_list.end();

    for (auto itr = window_list.begin(); itr != window_list.end(); ++itr) {
        if ((*itr)->full_translated_rect().contains(m_cursor.location())) {
            window_that_should_be_focused = itr;
            break;
        }
    }

    if (window_that_should_be_focused != window_list.end()) {
        if (Window::is_any_focused() && ((*window_that_should_be_focused).get() == &Window::focused()))
            return;

        (*window_that_should_be_focused)->set_focused();

        // Move the focused window to the front of window list
        window_list.splice(window_list.begin(), window_list, window_that_should_be_focused);
    } else if (!WindowManager::the().desktop()->is_focused()) {
        WindowManager::the().desktop()->set_focused();
    }
}

Point Screen::cursor_position_for_delta(i16 delta_x, i16 delta_y)
{
    bool left_movement = delta_x < 0;
    bool down_movement = delta_y < 0;

    size_t new_x = 0;
    size_t new_y = 0;

    auto current_x = m_shadow_cursor_location.x();
    auto current_y = m_shadow_cursor_location.y();

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

    Point new_position = { new_x, new_y };

    m_shadow_cursor_location = new_position;

    return new_position;
}
}
