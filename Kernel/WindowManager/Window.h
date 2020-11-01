#pragma once

#include "Bitmap.h"
#include "Common/DynamicArray.h"
#include "Event.h"
#include "Multitasking/Thread.h"
#include "Rect.h"
#include "Theme.h"
#include "WindowFrame.h"

namespace kernel {

class Window {
    MAKE_NONCOPYABLE(Window);
    MAKE_NONMOVABLE(Window);

public:
    enum class Style {
        FRAMELESS,
        NORMAL_FRAME
    };

    static RefPtr<Window> create(Thread& owner, const Rect& window_rect, RefPtr<Theme>);
    static RefPtr<Window> create_desktop(Thread& owner, const Rect& window_rect, RefPtr<Theme>);

    const RefPtr<Theme>& theme() const { return m_theme; }

    Rect full_rect() const { return m_frame.rect(); }
    const Rect& window_rect() const { return m_window_rect; }
    Rect view_rect() const { return m_frame.view_rect(); }
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

    void invalidate_part_of_view_rect(const Rect&);
    void submit_dirty_rects();

    void push_window_event(const Event&);

    InterruptSafeSpinLock& event_queue_lock() { return m_event_queue_lock; }
    DynamicArray<Event>& event_queue() { return m_event_queue; }

private:
    void invalidate_rects_based_on_drag_delta(const Rect& new_rect);

    Window(Thread& owner, Style, const Rect& window_rect, RefPtr<Theme>);

    enum class State {
        NORMAL,
        IS_BEING_DRAGGED,
        CLOSE_BUTTON_HOVERED,
        MAXIMIZE_BUTTON_HOVERED,
        MINIMIZE_BUTTON_HOVERED,
    };

private:
    Thread& m_owner;

    RefPtr<Theme> m_theme;

    Style m_style;
    Rect m_window_rect;
    WindowFrame m_frame;
    Point m_location;
    bool m_invalidated { true };
    State m_state { State::NORMAL };

    RefPtr<Surface> m_front_surface;

    Point m_drag_begin;

    InterruptSafeSpinLock m_dirty_rect_lock;
    DynamicArray<Rect> m_dirty_rects;

    InterruptSafeSpinLock m_event_queue_lock;
    DynamicArray<Event> m_event_queue;

    // TODO: this should be a weak ptr
    static Window* s_currently_focused;
};
}
