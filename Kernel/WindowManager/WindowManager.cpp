#include "Multitasking/Process.h"
#include "Multitasking/Sleep.h"

#include "Compositor.h"
#include "EventManager.h"
#include "Screen.h"
#include "WindowManager.h"

namespace kernel {

WindowManager* WindowManager::s_instance;

void WindowManager::initialize()
{
    ASSERT(s_instance == nullptr);

    Screen::initialize(VideoDevice::the());
    s_instance = new WindowManager();
    Compositor::initialize();
    Process::create_supervisor(&WindowManager::run, 4 * MB);
}

WindowManager::WindowManager()
    : m_theme(Theme::make_default())
    , m_desktop_window(Window::create_desktop(*Thread::current(), Screen::the().rect(), m_theme))
{
}

void WindowManager::run()
{
    auto& self = the();

    for (;;) {
        // Don't hold the lock while sleeping
        {
            LockGuard lock_guard(self.window_lock());

            EventManager::the().dispatch_pending();

            Compositor::the().compose();
        }

        sleep::for_milliseconds(1000 / 100);
    }
}
}
