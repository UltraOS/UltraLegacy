#pragma once

#include "Common/DynamicArray.h"
#include "Event.h"
#include "Multitasking/Thread.h"
#include "Utilities.h"

namespace kernel {

class Window {
public:
    RefPtr<Window> create(Thread& owner, const Rect& window_rect) { return new Window(owner, window_rect); }

    const Rect& rect() const { return m_rect; }

    void set_focused() { s_currently_focused = this; }

    static bool is_any_focused() { return s_currently_focused != nullptr; }

    static Window& focused()
    {
        ASSERT(s_currently_focused != nullptr);
        return *s_currently_focused;
    }

    void enqueue_event(const Event& event) { m_event_queue.append(event); }

private:
    Window(Thread& owner, const Rect& window_rect);

private:
    Thread&             m_owner;
    Rect                m_rect;
    u8*                 m_front_bitmap;
    DynamicArray<Event> m_event_queue;

    // TODO: this should be a weak ptr
    static Window*               s_currently_focused;
    static InterruptSafeSpinLock s_lock;
};
}
