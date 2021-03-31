#include "Multitasking/Process.h"
#include "Multitasking/Sleep.h"

#include "Compositor.h"
#include "EventManager.h"
#include "Memory/MemoryManager.h"
#include "Screen.h"
#include "WindowManager.h"

namespace kernel {

WindowManager* WindowManager::s_instance;

void WindowManager::initialize()
{
    ASSERT(s_instance == nullptr);

    Screen::initialize(VideoDevice::primary());
    s_instance = new WindowManager();
    Compositor::initialize();
    Process::create_supervisor(&WindowManager::run, "window manager"_sv, 4 * MB);
}

WindowManager::WindowManager()
    : m_theme(Theme::make_default())
    , m_desktop_window(Window::create_desktop(*Thread::current(), Screen::the().rect(), m_theme))
{
}

void WindowManager::add_window(const RefPtr<Window>& window)
{
    LOCK_GUARD(window_lock());
    m_windows.insert_front(*window);
    window->set_focused();
    Compositor::the().add_dirty_rect(window->full_translated_rect());
}

void WindowManager::remove_window(Window& window)
{
    LOCK_GUARD(window_lock());
    window.pop_off();

    if (Window::is_any_focused() && &window == &Window::focused()) {
        if (m_windows.empty())
            m_desktop_window->set_focused();
        else
            m_windows.front().set_focused();
    }

    Compositor::the().add_dirty_rect(window.full_translated_rect());
}

void WindowManager::run()
{
    auto& self = the();

    static constexpr auto nanoseconds_between_frames = Time::nanoseconds_in_second / 60;

    auto next_frame_time = Timer::nanoseconds_since_boot() + nanoseconds_between_frames;

    for (;;) {
        // Don't hold the lock while sleeping
        {
            LOCK_GUARD(self.window_lock());

            EventManager::the().dispatch_pending();

            Compositor::the().compose();
        }

        while (Timer::nanoseconds_since_boot() < next_frame_time)
            continue;

        next_frame_time = Timer::nanoseconds_since_boot() + nanoseconds_between_frames;
    }
}
}
