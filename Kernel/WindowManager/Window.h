#pragma once

#include "Drivers/Video/Utilities.h"
#include "Multitasking/Thread.h"

namespace kernel {

class Window {
public:
    RefPtr<Window> create(Thread& owner, const Rect& window_rect) { return new Window(owner, window_rect); }

    const Rect& rect() const { return m_rect; }

private:
    Window(Thread& owner, const Rect& window_rect);

private:
    Thread& m_owner;
    Rect    m_rect;
    u8*     m_front_bitmap;

    // TODO: this should be a weak ptr
    static Window*               s_currently_focused;
    static InterruptSafeSpinLock s_lock;
};
}
