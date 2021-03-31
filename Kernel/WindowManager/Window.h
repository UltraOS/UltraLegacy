#pragma once

#include "Bitmap.h"
#include "Common/CircularBuffer.h"
#include "Common/DynamicArray.h"
#include "Common/List.h"
#include "Event.h"
#include "Memory/VirtualRegion.h"
#include "Rect.h"
#include "Theme.h"
#include "WindowFrame.h"

namespace kernel {

class Thread;

class Window : public StandaloneListNode<Window> {
    MAKE_NONCOPYABLE(Window);
    MAKE_NONMOVABLE(Window);

public:
    enum class Style {
        FRAMELESS,
        NORMAL_FRAME
    };

    static RefPtr<Window> create(Thread& owner, const Rect& window_rect, RefPtr<Theme>, StringView title = "Unnamed"_sv);
    static RefPtr<Window> create_desktop(Thread& owner, const Rect& window_rect, RefPtr<Theme>);

    [[nodiscard]] const Theme& theme() const { return *m_theme; }

    [[nodiscard]] Rect full_rect() const { return m_frame.rect(); }
    [[nodiscard]] const Rect& window_rect() const { return m_window_rect; }
    [[nodiscard]] Rect view_rect() const { return m_frame.view_rect(); }
    [[nodiscard]] const Point& location() const { return m_location; }

    [[nodiscard]] Rect full_translated_rect() const { return m_frame.rect().translated(m_location); }

    [[nodiscard]] bool has_frame() const { return m_style != Style::FRAMELESS; }

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

    [[nodiscard]] const Surface& surface() const
    {
        ASSERT(m_front_surface.get() != nullptr);
        return *m_front_surface;
    }

    [[nodiscard]] size_t width() const { return surface().width(); }
    [[nodiscard]] size_t height() const { return surface().height(); }

    bool handle_event(const Event& event, bool is_handled = false);

    [[nodiscard]] bool is_invalidated() const { return m_invalidated; }
    void set_invalidated(bool setting) { m_invalidated = setting; }

    void invalidate_part_of_view_rect(const Rect&);
    void invalidate_rect(const Rect&);
    void submit_dirty_rects();

    void push_window_event(const Event&);

    Event pop_event()
    {
        LOCK_GUARD(m_event_queue_lock);

        if (m_event_queue.empty()) {
            Event e {};
            e.type = Event::Type::EMPTY;
            return e;
        }

        return m_event_queue.dequeue();
    }

    [[nodiscard]] const String& title() const { return m_title; }

    VirtualRegion& surface_region() { return *m_surface_region; }

    void close();

    ~Window() { ASSERT(m_state == State::CLOSED); }

    friend bool operator<(const RefPtr<Window>& l, const Window* r)
    {
        return l.get() < r;
    }

    friend bool operator<(const Window* l, const RefPtr<Window>& r)
    {
        return l < r.get();
    }

private:
    void invalidate_rects_based_on_drag_delta(const Rect& new_rect) const;

    Window(Thread& owner, Style, const Rect& window_rect, RefPtr<Theme>, StringView title);

    enum class State {
        NORMAL,
        IS_BEING_DRAGGED,
        CLOSE_BUTTON_HOVERED,
        CLOSE_BUTTON_PRESSED,
        MAXIMIZE_BUTTON_HOVERED,
        MINIMIZE_BUTTON_HOVERED,
        CLOSED,
    };

private:
    Thread& m_owner;

    String m_title;

    RefPtr<VirtualRegion> m_surface_region;
    RefPtr<Theme> m_theme;

    Style m_style;
    Rect m_window_rect;
    WindowFrame m_frame;
    Point m_location;
    // TODO: set this to false and invalidate the entire window
    // with invalidate_rect() upon creation instead?
    bool m_invalidated { false };
    State m_state { State::NORMAL };

    RefPtr<Surface> m_front_surface;

    Point m_drag_begin;

    InterruptSafeSpinLock m_dirty_rect_lock;
    DynamicArray<Rect> m_dirty_rects;

    static constexpr size_t max_event_queue_size = 16;
    InterruptSafeSpinLock m_event_queue_lock;
    CircularBuffer<Event, max_event_queue_size> m_event_queue;

    static Window* s_currently_focused;
};
}
