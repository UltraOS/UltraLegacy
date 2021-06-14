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

// FIXME: use safe_memcpy etc once we have it
ErrorCode WindowManager::dispatch_window_command(void* user_ptr)
{
    auto command = *Address(user_ptr).as_pointer<WMCommand>();

    switch (command) {
    case WMCommand::CREATE_WINDOW: {
        WindowCreateCommand wc_command {};
        copy_memory(user_ptr, &wc_command, sizeof(WindowCreateCommand));

        auto* current_thread = Thread::current();

        Rect window_rect(
                static_cast<ssize_t>(wc_command.top_left_x),
                static_cast<ssize_t>(wc_command.top_left_y),
                static_cast<ssize_t>(wc_command.width),
                static_cast<ssize_t>(wc_command.height));

        auto window =  Window::create(*current_thread, window_rect, WindowManager::the().active_theme(), wc_command.title);

        wc_command.window_id = current_thread->add_window(window);

        // TODO: don't generate surface regions inside kernel address space :(
        Address fb_address = window->surface_region().virtual_range().begin();

        if (window->has_frame()) {
            const auto& theme = window->theme();

            //               upper border -> -------
            //               left border ->  |     | <- right border
            //                                ^- start of user framebuffer
            wc_command.pitch = theme.side_window_frame_width() * 2 + window->width();
            wc_command.pitch *= sizeof(u32);

            // Skip the upper frame + left border of the first row.
            fb_address += theme.upper_window_frame_height() * wc_command.pitch;
            fb_address += theme.side_window_frame_width() * sizeof(u32);
        } else {
            wc_command.pitch = window->width() * sizeof(u32);
        }

        wc_command.framebuffer = fb_address.as_pointer<void>();

        copy_memory(&wc_command, user_ptr, sizeof(WindowCreateCommand));

        return ErrorCode::NO_ERROR;
    }
    case WMCommand::DESTROY_WINDOW: {
        WindowDestroyCommand wd_command {};
        copy_memory(user_ptr, &wd_command, sizeof(WindowDestroyCommand));

        auto* current_thread = Thread::current();
        auto& windows = current_thread->windows();
        auto this_window = windows.find(wd_command.window_id);

        if (this_window == windows.end())
            return ErrorCode::INVALID_ARGUMENT;

        this_window->second->close();
        return ErrorCode::NO_ERROR;
    }
    case WMCommand::POP_EVENT: {
        PopEventCommand pe_command {};
        copy_memory(user_ptr, &pe_command, sizeof(PopEventCommand));

        auto* current_thread = Thread::current();
        auto& windows = current_thread->windows();
        auto this_window = windows.find(pe_command.window_id);

        if (this_window == windows.end())
            return ErrorCode::INVALID_ARGUMENT;

        pe_command.event = this_window->second->pop_event();
        copy_memory(&pe_command, user_ptr, sizeof(PopEventCommand));
        return ErrorCode::NO_ERROR;
    }
    case WMCommand::SET_TITLE: {
        SetTitleCommand st_command {};
        copy_memory(user_ptr, &st_command, sizeof(SetTitleCommand));

        auto* current_thread = Thread::current();
        auto& windows = current_thread->windows();
        auto this_window = windows.find(st_command.window_id);

        if (this_window == windows.end())
            return ErrorCode::INVALID_ARGUMENT;

        // FIXME: This is insanely unsafe :D
        this_window->second->set_title(st_command.title);
        return ErrorCode::NO_ERROR;
    }
    default:
        return ErrorCode::INVALID_ARGUMENT;
    }
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
