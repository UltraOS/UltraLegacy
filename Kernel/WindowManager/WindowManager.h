#pragma once

#include "Common/List.h"
#include "Theme.h"
#include "Window.h"

#include "Core/ErrorCode.h"
#include <Shared/WMCommands.h>

namespace kernel {

enum class WMCommand : u32 {
#define WM_COMMAND(name) name,
    ENUMERATE_WM_COMMANDS
#undef WM_COMMAND
};

#define WM_COMMAND_TYPE WMCommand

#include <Shared/Window.h>

class WindowManager {
    MAKE_SINGLETON(WindowManager);

public:
    static void initialize();

    void add_window(const RefPtr<Window>& window);

    void remove_window(Window& window);

    [[noreturn]] static void run();

    [[nodiscard]] RefPtr<Theme> active_theme() const { return m_theme; }

    InterruptSafeSpinLock& window_lock() { return m_lock; }

    List<Window>& windows() { return m_windows; }

    static WindowManager& the()
    {
        ASSERT(s_instance != nullptr);
        return *s_instance;
    }

    ErrorCode dispatch_window_command(void*);

    RefPtr<Window>& desktop() { return m_desktop_window; }

private:
    InterruptSafeSpinLock m_lock;

    RefPtr<Theme> m_theme;
    RefPtr<Window> m_desktop_window;
    List<Window> m_windows;

    static WindowManager* s_instance;
};

}
