#include "WindowFrame.h"
#include "Painter.h"
#include "Window.h"

namespace kernel {

WindowFrame::WindowFrame(Window& owner) : m_owner(owner) { }

void WindowFrame::paint()
{
    if (!m_owner.has_frame())
        return;

    Painter painter(&m_owner.surface());

    auto frame_rect = rect();

    painter.fill_rect({ frame_rect.top_left(), frame_rect.width(), upper_frame_height }, default_frame_color);
}

Rect WindowFrame::rect() const
{
    if (!m_owner.has_frame())
        return m_owner.window_rect();

    auto final_rect = m_owner.window_rect();

    final_rect.set_height(final_rect.height() + upper_frame_height);

    return final_rect;
}

Rect WindowFrame::view_rect() const
{
    if (!m_owner.has_frame())
        return m_owner.window_rect();

    return m_owner.window_rect().translated(0, upper_frame_height);
}

Rect WindowFrame::translated_top_rect() const
{
    auto final_rect = rect().translated(m_owner.location());
    final_rect.set_height(upper_frame_height);

    return final_rect;
}
}
