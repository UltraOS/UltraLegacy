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
    Process::create_supervisor(&WindowManager::run, "window manager"_sv, 4 * MB);
}

WindowManager::WindowManager()
    : m_theme(Theme::make_default())
    , m_desktop_window(Window::create_desktop(*Thread::current(), Screen::the().rect(), m_theme))
{
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
