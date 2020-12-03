#include "Window.h"
#include "Common/Math.h"
#include "Compositor.h"
#include "Memory/MemoryManager.h"
#include "Screen.h"
#include "WindowManager.h"

namespace kernel {

Window* Window::s_currently_focused;

RefPtr<Window> Window::create(Thread& owner, const Rect& window_rect, RefPtr<Theme> theme, StringView title)
{
    RefPtr<Window> window = new Window(owner, Style::NORMAL_FRAME, window_rect, theme, title);
    WindowManager::the().add_window(window);
    return window;
}

RefPtr<Window> Window::create_desktop(Thread& owner, const Rect& window_rect, RefPtr<Theme> theme)
{
    return new Window(owner, Style::FRAMELESS, window_rect, theme, "Desktop"_sv);
}

Window::Window(Thread& owner, Style style, const Rect& window_rect, RefPtr<Theme> theme, StringView title)
    : m_owner(owner)
    , m_title(title)
    , m_theme(theme)
    , m_style(style)
    , m_window_rect(0, 0, window_rect.width(), window_rect.height())
    , m_frame(*this)
    , m_location(window_rect.top_left())
{
    auto full_window_rect = full_rect();
    auto pages_needed = ceiling_divide(full_window_rect.width() * full_window_rect.height() * sizeof(u32), Page::size);

    auto bitmap_range = AddressSpace::current().allocator().allocate(pages_needed * Page::size);
    auto bitmap = bitmap_range.as_pointer<void>();

    MemoryManager::the().force_preallocate(bitmap_range);

    m_front_surface = RefPtr<Surface>::create(
        bitmap,
        full_window_rect.width(),
        full_window_rect.height(),
        Surface::Format::RGBA_32_BPP);

    m_frame.paint();
}

void Window::invalidate_part_of_view_rect(const Rect& rect)
{
    LockGuard lock_guard(m_dirty_rect_lock);
    m_dirty_rects.append(rect.translated(view_rect().top_left()));
}

void Window::invalidate_rect(const Rect& rect)
{
    LockGuard lock_guard(m_dirty_rect_lock);
    m_dirty_rects.append(rect);
}

void Window::submit_dirty_rects()
{
    LockGuard lock_guard(m_dirty_rect_lock);

    for (const auto& rect : m_dirty_rects) {
        Compositor::the().add_dirty_rect(rect.translated(location()));
    }

    m_dirty_rects.clear();
}

// clang-format off
void Window::invalidate_rects_based_on_drag_delta(const Rect& new_rect) const
{
    enum class DragDirection {
        NONE,
        LEFT,
        RIGHT,
        UP,
        DOWN,
        UP_LEFT,
        DOWN_LEFT,
        UP_RIGHT,
        DOWN_RIGHT
    };

    auto drag_direction = [](const Rect& old_rect, const Rect& new_rect) -> DragDirection {
        if (old_rect == new_rect)
            return DragDirection::NONE;

        // up or down
        if (old_rect.left() == new_rect.left()) {
            ASSERT(old_rect.top() != new_rect.top());
            return old_rect.top() > new_rect.top() ? DragDirection::UP : DragDirection::DOWN;
        }

        // left or right
        if (old_rect.top() == new_rect.top()) {
            ASSERT(old_rect.left() != new_rect.left());
            return old_rect.left() > new_rect.left() ? DragDirection::LEFT : DragDirection::RIGHT;
        }

        // up left, up right
        if (old_rect.top() > new_rect.top()) {
            return old_rect.left() > new_rect.left() ? DragDirection::UP_LEFT : DragDirection::UP_RIGHT;
        }

        // down left, down right
        return old_rect.left() > new_rect.left() ? DragDirection::DOWN_LEFT : DragDirection::DOWN_RIGHT;
    };

    auto old_rect = full_translated_rect();

    switch (drag_direction(old_rect, new_rect)) {
    case DragDirection::NONE:
        return;
    case DragDirection::LEFT:
        Compositor::the().add_dirty_rect({
            new_rect.top_right(),
            old_rect.right() - new_rect.right() + 1,
            new_rect.height()
        });
        break;
    case DragDirection::RIGHT:
        Compositor::the().add_dirty_rect({
            old_rect.top_left(),
            new_rect.left() - old_rect.left() + 1,
            new_rect.height()
        });
        break;
    case DragDirection::UP:
        Compositor::the().add_dirty_rect({
            new_rect.bottom_left(),
            new_rect.width(),
            old_rect.bottom() - new_rect.bottom() + 1
        });
        break;
    case DragDirection::DOWN:
        Compositor::the().add_dirty_rect({
            old_rect.top_left(),
            new_rect.width(),
            new_rect.top() - old_rect.top() + 1
        });
        break;
    case DragDirection::UP_LEFT:
        Compositor::the().add_dirty_rect({
            new_rect.right(),
            old_rect.top(),
            old_rect.right() - new_rect.right() + 1,
            new_rect.bottom() - old_rect.top() + 1
        });
        Compositor::the().add_dirty_rect({
            old_rect.left(),
            new_rect.bottom(),
            old_rect.width(),
            old_rect.bottom() - new_rect.bottom() + 1
        });
        break;
    case DragDirection::DOWN_LEFT:
        Compositor::the().add_dirty_rect({
            old_rect.top_left(),
            old_rect.width(),
            new_rect.top() - old_rect.top() + 1
        });
        Compositor::the().add_dirty_rect({
            new_rect.top_right(),
            old_rect.right() - new_rect.right() + 1,
            old_rect.bottom() - new_rect.top() + 1
        });
        break;
    case DragDirection::UP_RIGHT:
        Compositor::the().add_dirty_rect({
            old_rect.top_left(),
            new_rect.left() - old_rect.left() + 1,
            new_rect.bottom() - old_rect.top() + 1
        });
        Compositor::the().add_dirty_rect({
            old_rect.left(),
            new_rect.bottom(),
            old_rect.width(),
            old_rect.bottom() - new_rect.bottom() + 1
        });
        break;
    case DragDirection::DOWN_RIGHT:
        Compositor::the().add_dirty_rect({
            old_rect.top_left(),
            old_rect.width(),
            new_rect.top() - old_rect.top() + 1
        });
        Compositor::the().add_dirty_rect({
            old_rect.left(),
            new_rect.top(),
            new_rect.left() - old_rect.left() + 1,
            new_rect.bottom() - new_rect.top() + 1
        });
        break;
    default:
        ASSERT_NEVER_REACHED();
    }
}
// clang-format on

void Window::push_window_event(const Event& event)
{
    static constexpr size_t max_event_queue_size = 16;

    LockGuard lock_guard(m_event_queue_lock);

    if (m_event_queue.size() >= max_event_queue_size)
        return;

    m_event_queue.append(event);
}

bool Window::handle_event(const Event& event, bool is_handled)
{
    if (event.type == Event::Type::MOUSE_MOVE) {
        if (!is_handled) {
            auto mouse_vec = Point(event.mouse_move.x, event.mouse_move.y);

            auto view_rectangle = view_rect();

            if (view_rectangle.translated(location()).contains(mouse_vec)) {
                Event user_event = event;
                mouse_vec.move_by(-m_location.x(), -m_location.y());
                mouse_vec.move_by(-view_rectangle.left(), -view_rectangle.top());
                user_event.mouse_move = { static_cast<size_t>(mouse_vec.x()), static_cast<size_t>(mouse_vec.y()) };
                push_window_event(user_event);
            }
        }
    } else
        push_window_event(event);

    switch (event.type) {
    case Event::Type::BUTTON_STATE: {
        auto key = event.vk_state.vkey;
        auto key_state = event.vk_state.state;

        if (key == VK::MOUSE_LEFT_BUTTON) {
            if (key_state == VKState::PRESSED) {
                if (m_frame.draggable_rect().contains(Screen::the().cursor().location())) {
                    m_state = State::IS_BEING_DRAGGED;
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
                m_frame.on_button_state_changed(WindowFrame::Button::CLOSE, WindowFrame::ButtonState::RELEASED);
            } else if (m_state == State::MAXIMIZE_BUTTON_HOVERED) {
                m_state = State::NORMAL;
                m_frame.on_button_state_changed(WindowFrame::Button::MAXIMIZE, WindowFrame::ButtonState::RELEASED);
            } else if (m_state == State::MINIMIZE_BUTTON_HOVERED) {
                m_state = State::NORMAL;
                m_frame.on_button_state_changed(WindowFrame::Button::MINIMIZE, WindowFrame::ButtonState::RELEASED);
            }
        };

        auto mouse_vec = Point(event.mouse_move.x, event.mouse_move.y);

        if (m_state == State::IS_BEING_DRAGGED) {
            ssize_t delta_x = static_cast<ssize_t>(event.mouse_move.x) - static_cast<ssize_t>(m_drag_begin.x());
            ssize_t delta_y = static_cast<ssize_t>(event.mouse_move.y) - static_cast<ssize_t>(m_drag_begin.y());

            ssize_t new_x = m_location.x() + delta_x;
            ssize_t new_y = m_location.y() + delta_y;

            Rect new_window_rect = full_translated_rect();
            new_window_rect.set_top_left_x(new_x);
            new_window_rect.set_top_left_y(new_y);
            invalidate_rects_based_on_drag_delta(new_window_rect);

            m_location = Point(new_x, new_y);
            m_drag_begin = { static_cast<ssize_t>(event.mouse_move.x), static_cast<ssize_t>(event.mouse_move.y) };

            set_invalidated(true);
        } else if (m_frame.rect_for_button(WindowFrame::Button::CLOSE).contains(mouse_vec) && !is_handled) {
            if (m_state != State::CLOSE_BUTTON_HOVERED) {
                release_buttons_if_needed();
                m_state = State::CLOSE_BUTTON_HOVERED;
                m_frame.on_button_state_changed(WindowFrame::Button::CLOSE, WindowFrame::ButtonState::HOVERED);
            }
        } else if (m_frame.rect_for_button(WindowFrame::Button::MAXIMIZE).contains(mouse_vec) && !is_handled) {
            if (m_state != State::MAXIMIZE_BUTTON_HOVERED) {
                release_buttons_if_needed();
                m_state = State::MAXIMIZE_BUTTON_HOVERED;
                m_frame.on_button_state_changed(WindowFrame::Button::MAXIMIZE, WindowFrame::ButtonState::HOVERED);
            }
        } else if (m_frame.rect_for_button(WindowFrame::Button::MINIMIZE).contains(mouse_vec) && !is_handled) {
            if (m_state != State::MINIMIZE_BUTTON_HOVERED) {
                release_buttons_if_needed();
                m_state = State::MINIMIZE_BUTTON_HOVERED;
                m_frame.on_button_state_changed(WindowFrame::Button::MINIMIZE, WindowFrame::ButtonState::HOVERED);
            }
        } else {
            release_buttons_if_needed();
            if (!full_translated_rect().contains(mouse_vec))
                return false;
        }
        return true;
    }
    default:
        return true;
    }
}
}
