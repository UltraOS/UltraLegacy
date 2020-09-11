#include "Window.h"

namespace kernel {

Window*               Window::s_currently_focused;
InterruptSafeSpinLock Window::s_lock;

Window::Window(Thread& owner, const Rect& window_rect) : m_owner(owner), m_rect(window_rect)
{
    m_front_bitmap = AddressSpace::current()
                         .allocator()
                         .allocate_range(window_rect.width() * window_rect.height() * sizeof(u32))
                         .as_pointer<u8>();
}

}
