#include "Multitasking/Process.h"
#include "Multitasking/Sleep.h"

#include "Compositor.h"
#include "EventManager.h"
#include "Memory/MemoryManager.h"
#include "Memory/SafeOperations.h"
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

ErrorCode WindowManager::dispatch_window_command(void* user_ptr)
{
    WMCommand command;
    if (!safe_copy_memory(user_ptr, &command, sizeof(command)))
        return ErrorCode::MEMORY_ACCESS_VIOLATION;

    switch (command) {
    case WMCommand::CREATE_WINDOW: {
        WindowCreateCommand wc_command {};
        if (!safe_copy_memory(user_ptr, &wc_command, sizeof(WindowCreateCommand)))
            return ErrorCode::MEMORY_ACCESS_VIOLATION;

        auto* current_thread = Thread::current();

        Rect window_rect(
                static_cast<ssize_t>(wc_command.top_left_x),
                static_cast<ssize_t>(wc_command.top_left_y),
                static_cast<ssize_t>(wc_command.width),
                static_cast<ssize_t>(wc_command.height));

        auto window =  Window::create(*current_thread, window_rect, WindowManager::the().active_theme(), wc_command.title);
        wc_command.window_id = window->id();

        auto user_window_region =
                MemoryManager::the().allocate_user_shared(
                        static_cast<SharedVirtualRegion&>(window->surface_region()),
                        AddressSpace::current());

        auto& this_process = Process::current();
        this_process.store_region(user_window_region);

        auto fb_address = user_window_region->virtual_range().begin();

        if (window->has_frame()) {
            const auto& theme = window->theme();

            //               upper border -> -------
            //               left border ->  |     | <- right border
            //                                ^- start of user framebuffer
            wc_command.pitch = window->width() * sizeof(u32);

            // Skip the upper frame + left border of the first row.
            fb_address += theme.upper_window_frame_height() * wc_command.pitch;
            fb_address += theme.side_window_frame_width() * sizeof(u32);
        } else {
            wc_command.pitch = window->width() * sizeof(u32);
        }

        wc_command.framebuffer = fb_address.as_pointer<void>();

        if (!safe_copy_memory(&wc_command, user_ptr, sizeof(WindowCreateCommand))) {
            window->close();
            current_thread->windows().remove(wc_command.window_id);
            return ErrorCode::MEMORY_ACCESS_VIOLATION;
        }

        return ErrorCode::NO_ERROR;
    }
    case WMCommand::DESTROY_WINDOW: {
        WindowDestroyCommand wd_command {};
        if (!safe_copy_memory(user_ptr, &wd_command, sizeof(WindowDestroyCommand)))
            return ErrorCode::MEMORY_ACCESS_VIOLATION;

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
        if (!safe_copy_memory(user_ptr, &pe_command, sizeof(PopEventCommand)))
            return ErrorCode::MEMORY_ACCESS_VIOLATION;

        auto* current_thread = Thread::current();
        auto& windows = current_thread->windows();
        auto this_window = windows.find(pe_command.window_id);

        if (this_window == windows.end())
            return ErrorCode::INVALID_ARGUMENT;

        pe_command.event = this_window->second->pop_event();
        if (!safe_copy_memory(&pe_command, user_ptr, sizeof(PopEventCommand)))
            return ErrorCode::MEMORY_ACCESS_VIOLATION;

        return ErrorCode::NO_ERROR;
    }
    case WMCommand::SET_TITLE: {
        SetTitleCommand st_command {};
        if (!safe_copy_memory(user_ptr, &st_command, sizeof(SetTitleCommand)))
            return ErrorCode::MEMORY_ACCESS_VIOLATION;

        auto* current_thread = Thread::current();
        auto& windows = current_thread->windows();
        auto this_window = windows.find(st_command.window_id);

        if (this_window == windows.end())
            return ErrorCode::INVALID_ARGUMENT;

        char title_buffer[256];
        size_t bytes = copy_until_null_or_n_from_user(st_command.title, title_buffer, 256);
        StringView title_view;

        if (bytes == 0)
            return ErrorCode::MEMORY_ACCESS_VIOLATION;
        else if (bytes == 1)
            title_view = "Untitled"_sv;
        else
            title_view = StringView(title_buffer, bytes - 1);

        this_window->second->set_title(title_view);
        return ErrorCode::NO_ERROR;
    }
    case WMCommand::INVALIDATE_RECT: {
        InvalidateRectCommand ir_command {};
        if (!safe_copy_memory(user_ptr, &ir_command, sizeof(InvalidateRectCommand)))
            return ErrorCode::MEMORY_ACCESS_VIOLATION;

        auto* current_thread = Thread::current();
        auto& windows = current_thread->windows();
        auto this_window = windows.find(ir_command.window_id);

        if (this_window == windows.end())
            return ErrorCode::INVALID_ARGUMENT;

        auto& window = this_window->second;

        window->invalidate_rect(window->view_rect());

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
