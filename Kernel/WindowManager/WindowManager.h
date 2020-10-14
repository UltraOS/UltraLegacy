#pragma once

#include "Common/List.h"
#include "Window.h"

namespace kernel {

class WindowManager {
public:
    static void initialize();

    void add_window(const RefPtr<Window>& window)
    {
        LockGuard lock_guard(m_lock);
        m_windows.append_front(window);
    }

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
