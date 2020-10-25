#pragma once

#include "Bitmap.h"
#include "Common/DynamicArray.h"
#include "Event.h"
#include "Multitasking/Thread.h"
#include "Rect.h"
#include "WindowFrame.h"

namespace kernel {

class Window {
    MAKE_NONCOPYABLE(Window);
    MAKE_NONMOVABLE(Window);

public:
    enum class Style { FRAMELESS, NORMAL_FRAME };

    static RefPtr<Window> create(Thread& owner, const Rect& window_rect);
    static RefPtr<Window> create_desktop(Thread& owner, const Rect& window_rect);

    Rect         full_rect() const { return m_frame.rect(); }
    const Rect&  window_rect() const { return m_window_rect; }
    Rect         view_rect() const { return m_frame.view_rect(); }
    const Point& location() const { return m_location; }

    Rect full_translated_rect() const { return m_frame.rect().translated(m_location); }

    bool has_frame() const { return m_style != Style::FRAMELESS; }

    void set_focused() { s_currently_focused = this; }

    bool is_focused() { return s_currently_focused == this; }

    static bool is_any_focused() { return s_currently_focused != nullptr; }

    static Window& focused()
    {
        ASSERT(s_currently_focused != nullptr);
        return *s_currently_focused;
    }

    Surface& surface()
    {
        ASSERT(m_front_surface.get() != nullptr);
        return *m_front_surface;
    }

    const Surface& surface() const
    {
        ASSERT(m_front_surface.get() != nullptr);
        return *m_front_surface;
    }

    size_t width() const { return surface().width(); }
    size_t height() const { return surface().height(); }

    bool handle_event(const Event& event, bool is_handled = false);

    bool is_invalidated() { return m_invalidated; }
    void set_invalidated(bool setting) { m_invalidated = setting; }

private:
    Window(Thread& owner, Style, const Rect& window_rect);

    enum class State {
        NORMAL,
        IS_BEING_DRAGGED,
        CLOSE_BUTTON_HOVERED,
        MAXIMIZE_BUTTON_HOVERED,
        MINIMIZE_BUTTON_HOVERED,
    };

private:
    Thread& m_owner;

    Style       m_style;
    Rect        m_window_rect;
    WindowFrame m_frame;
    Point       m_location;
    bool        m_invalidated { true };
    State       m_state { State::NORMAL };

    RefPtr<Surface> m_front_surface;

    Point m_drag_begin;

    // Should also be a list?
    DynamicArray<Event> m_event_queue;

    // TODO: this should be a weak ptr
    static Window* s_currently_focused;
};
}
