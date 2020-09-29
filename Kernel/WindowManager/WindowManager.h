#pragma once

#include "Common/DynamicArray.h"
#include "Window.h"

namespace kernel {

class WindowManager {
public:
    static void initialize();

    // TODO: add locking here
    void                                add_window(const RefPtr<Window>& window) { m_windows.emplace(window); }
    const DynamicArray<RefPtr<Window>>& windows() { return m_windows; }

    static WindowManager& the() { return s_instance; }

    RefPtr<Window>& desktop() { return m_desktop_window; }

private:
    RefPtr<Window> m_desktop_window;

    // TODO: this needs to be a linked list
    DynamicArray<RefPtr<Window>> m_windows;

    static WindowManager s_instance;
};

}
