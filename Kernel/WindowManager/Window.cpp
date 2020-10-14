#include "Window.h"
#include "Common/Math.h"
#include "Memory/MemoryManager.h"
#include "WindowManager.h"

namespace kernel {

Window*               Window::s_currently_focused;
InterruptSafeSpinLock Window::s_lock;

RefPtr<Window> Window::create(Thread& owner, const Rect& window_rect, bool is_focused)
{
    RefPtr<Window> window = new Window(owner, window_rect, is_focused);
    WindowManager::the().add_window(window);
    return window;
}

RefPtr<Window> Window::create_desktop(Thread& owner, const Rect& window_rect)
{
    return new Window(owner, window_rect, false);
}

Window::Window(Thread& owner, const Rect& window_rect, bool is_focused) : m_owner(owner), m_rect(window_rect)
{
    auto pages_needed = ceiling_divide(window_rect.width() * window_rect.height() * sizeof(u32), Page::size);

    auto bitmap_range = AddressSpace::current().allocator().allocate_range(pages_needed * Page::size);
    auto bitmap       = bitmap_range.as_pointer<void>();

    MemoryManager::the().force_preallocate(bitmap_range);

    m_front_surface
        = RefPtr<Surface>::create(bitmap, window_rect.width(), window_rect.height(), Surface::Format::RGBA_32_BPP);

    if (is_focused) {
        LockGuard lock_guard(focus_lock());
        set_focused();
    }
}
}
