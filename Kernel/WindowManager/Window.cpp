#include "Window.h"
#include "Common/Math.h"
#include "Compositor.h"
#include "Memory/MemoryManager.h"
#include "Screen.h"
#include "WindowManager.h"

namespace kernel {

Window* Window::s_currently_focused;

RefPtr<Window> Window::create(Thread& owner, const Rect& window_rect, RefPtr<Theme> theme)
{
    RefPtr<Window> window = new Window(owner, Style::NORMAL_FRAME, window_rect, theme);
    WindowManager::the().add_window(window);
    return window;
}

RefPtr<Window> Window::create_desktop(Thread& owner, const Rect& window_rect, RefPtr<Theme> theme)
{
    return new Window(owner, Style::FRAMELESS, window_rect, theme);
}

Window::Window(Thread& owner, Style style, const Rect& window_rect, RefPtr<Theme> theme)
    : m_owner(owner), m_theme(theme), m_style(style), m_window_rect(0, 0, window_rect.width(), window_rect.height()),
      m_frame(*this), m_location(window_rect.top_left())
{
    auto full_window_rect = full_rect();
    auto pages_needed = ceiling_divide(full_window_rect.width() * full_window_rect.height() * sizeof(u32), Page::size);

    auto bitmap_range = AddressSpace::current().allocator().allocate_range(pages_needed * Page::size);
    auto bitmap       = bitmap_range.as_pointer<void>();

    MemoryManager::the().force_preallocate(bitmap_range);

    m_front_surface = RefPtr<Surface>::create(bitmap,
                                              full_window_rect.width(),
                                              full_window_rect.height(),
                                              Surface::Format::RGBA_32_BPP);

    m_frame.paint();
}

bool Window::handle_event(const Event& event, bool is_handled)
{
    if (!has_frame()) {
        // Uncomment later when needed
        // m_event_queue.append(event);
        return true;
    }

    switch (event.type) {
    case Event::Type::BUTTON_STATE: {
        auto key       = event.vk_state.vkey;
        auto key_state = event.vk_state.state;

        if (key == VK::MOUSE_LEFT_BUTTON) {
            if (key_state == VKState::PRESSED) {
                if (m_frame.draggable_rect().contains(Screen::the().cursor().location())) {
                    m_state      = State::IS_BEING_DRAGGED;
                    m_drag_begin = Screen::the().cursor().location();
                } else {
                    m_state = State::NORMAL;
                }
            } else {
                m_state = State::NORMAL;
            }
        }

        return true;
    }
    case Event::Type::MOUSE_MOVE: {
        auto release_buttons_if_needed = [this]() {
            if (m_state == State::CLOSE_BUTTON_HOVERED) {
                m_state = State::NORMAL;
                m_frame.on_close_button_released();
            } else if (m_state == State::MAXIMIZE_BUTTON_HOVERED) {
                m_state = State::NORMAL;
                m_frame.on_maximize_button_released();
            } else if (m_state == State::MINIMIZE_BUTTON_HOVERED) {
                m_state = State::NORMAL;
                m_frame.on_minimize_button_released();
            }
        };

        if (m_state == State::IS_BEING_DRAGGED) {
            i32 delta_x = static_cast<i32>(event.mouse_move.x) - static_cast<i32>(m_drag_begin.x());
            i32 delta_y = static_cast<i32>(event.mouse_move.y) - static_cast<i32>(m_drag_begin.y());

            i32 new_x = static_cast<i32>(m_location.x()) + delta_x;
            i32 new_y = static_cast<i32>(m_location.y()) + delta_y;

            Compositor::the().add_dirty_rect(full_translated_rect());

            m_location   = Point(new_x, new_y);
            m_drag_begin = { event.mouse_move.x, event.mouse_move.y };
        } else if (m_frame.close_button_rect().contains({ event.mouse_move.x, event.mouse_move.y }) && !is_handled) {
            if (m_state != State::CLOSE_BUTTON_HOVERED) {
                release_buttons_if_needed();
                m_state = State::CLOSE_BUTTON_HOVERED;
                m_frame.on_close_button_hovered();
            }
        } else if (m_frame.maximize_button_rect().contains({ event.mouse_move.x, event.mouse_move.y }) && !is_handled) {
            if (m_state != State::MAXIMIZE_BUTTON_HOVERED) {
                release_buttons_if_needed();
                m_state = State::MAXIMIZE_BUTTON_HOVERED;
                m_frame.on_maximize_button_hovered();
            }
        } else if (m_frame.minimize_button_rect().contains({ event.mouse_move.x, event.mouse_move.y }) && !is_handled) {
            if (m_state != State::MINIMIZE_BUTTON_HOVERED) {
                release_buttons_if_needed();
                m_state = State::MINIMIZE_BUTTON_HOVERED;
                m_frame.on_minimize_button_hovered();
            }
        } else {
            release_buttons_if_needed();
            if (!full_translated_rect().contains({ event.mouse_move.x, event.mouse_move.y }))
                return false;
        }
        return true;
    }
    default: return true;
    }
}
}
