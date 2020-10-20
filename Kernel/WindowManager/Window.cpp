#include "Window.h"
#include "Common/Math.h"
#include "Compositor.h"
#include "Memory/MemoryManager.h"
#include "Screen.h"
#include "WindowManager.h"

namespace kernel {

Window*               Window::s_currently_focused;
InterruptSafeSpinLock Window::s_lock;

RefPtr<Window> Window::create(Thread& owner, const Rect& window_rect, bool is_focused)
{
    RefPtr<Window> window = new Window(owner, Style::NORMAL_FRAME, window_rect, is_focused);
    WindowManager::the().add_window(window);
    return window;
}

RefPtr<Window> Window::create_desktop(Thread& owner, const Rect& window_rect)
{
    return new Window(owner, Style::FRAMELESS, window_rect, false);
}

Window::Window(Thread& owner, Style style, const Rect& window_rect, bool is_focused)
    : m_owner(owner), m_style(style), m_window_rect(0, 0, window_rect.width(), window_rect.height()), m_frame(*this),
      m_location(window_rect.top_left())
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

    if (is_focused) {
        LockGuard lock_guard(focus_lock());
        set_focused();
    }
}

void Window::handle_event(const Event& event)
{
    if (!has_frame()) {
        // Uncomment later when needed
        // m_event_queue.append(event);
        return;
    }

    if (event.type == Event::Type::BUTTON_STATE) {
        auto key   = event.vk_state.vkey;
        auto state = event.vk_state.state;

        if (key == VK::MOUSE_LEFT_BUTTON) {
            if (state == VKState::PRESSED) {
                if (m_frame.draggable_rect().contains(Screen::the().cursor().location())) {
                    m_is_being_dragged = true;
                    m_drag_begin       = Screen::the().cursor().location();
                }
            } else {
                m_is_being_dragged = false;
            }
        }
    } else if (event.type == Event::Type::MOUSE_MOVE) {
        if (m_is_being_dragged) {
            i32 delta_x = static_cast<i32>(event.mouse_move.x) - static_cast<i32>(m_drag_begin.x());
            i32 delta_y = static_cast<i32>(event.mouse_move.y) - static_cast<i32>(m_drag_begin.y());

            i32 new_locx = static_cast<i32>(m_location.x()) + delta_x;
            i32 new_locy = static_cast<i32>(m_location.y()) + delta_y;

            Compositor::the().add_dirty_rect(full_translated_rect());

            LockGuard lock_guard(lock());
            m_location   = Point(new_locx, new_locy);
            m_drag_begin = { event.mouse_move.x, event.mouse_move.y };
        } else if (m_frame.close_button_rect().contains({ event.mouse_move.x, event.mouse_move.y })
                   && !m_close_button_hovered) {
            m_close_button_hovered = true;
            m_frame.on_close_button_hovered();
        } else if (!m_frame.close_button_rect().contains({ event.mouse_move.x, event.mouse_move.y })
                   && m_close_button_hovered) {
            m_close_button_hovered = false;
            m_frame.on_close_button_released();
        }
    }
}
}
