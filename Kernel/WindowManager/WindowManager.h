#pragma once

#include "Common/List.h"
#include "Window.h"

namespace kernel {

class WindowManager {
    MAKE_SINGLETON(WindowManager) = default;

public:
    static void initialize();

    void add_window(const RefPtr<Window>& window)
    {
        LockGuard lock_guard(window_lock());
        m_windows.append_front(window);
        window->set_focused();
    }

    static void run();

    InterruptSafeSpinLock& window_lock() { return m_lock; }

    List<RefPtr<Window>>& windows() { return m_windows; }

    static WindowManager& the() { return s_instance; }

    RefPtr<Window>& desktop() { return m_desktop_window; }

private:
    InterruptSafeSpinLock m_lock;

    RefPtr<Window>       m_desktop_window;
    List<RefPtr<Window>> m_windows;

    static WindowManager s_instance;
};

}
