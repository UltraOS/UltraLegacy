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

    auto& theme = m_owner.theme();

    auto frame_rect = rect();
    painter.fill_rect({ frame_rect.top_left(), frame_rect.width(), theme->upper_window_frame_height() },
                      theme->window_frame_color());
    on_close_button_released();
    on_maximize_button_released();
    on_minimize_button_released();
}

void WindowFrame::on_close_button_hovered()
{
    draw_close_button(m_owner.theme()->color_for_button_state(Theme::Button::CLOSE, Theme::ButtonState::HOVERED));
}

void WindowFrame::on_close_button_released()
{
    draw_close_button(m_owner.theme()->color_for_button_state(Theme::Button::CLOSE, Theme::ButtonState::RELEASED));
}

void WindowFrame::on_maximize_button_hovered()
{
    draw_maximize_button(m_owner.theme()->color_for_button_state(Theme::Button::MAXIMIZE, Theme::ButtonState::HOVERED));
}

void WindowFrame::on_maximize_button_released()
{
    draw_maximize_button(
        m_owner.theme()->color_for_button_state(Theme::Button::MAXIMIZE, Theme::ButtonState::RELEASED));
}

void WindowFrame::on_minimize_button_hovered()
{
    draw_minimize_button(m_owner.theme()->color_for_button_state(Theme::Button::MINIMIZE, Theme::ButtonState::HOVERED));
}

void WindowFrame::on_minimize_button_released()
{
    draw_minimize_button(
        m_owner.theme()->color_for_button_state(Theme::Button::MINIMIZE, Theme::ButtonState::RELEASED));
}

void WindowFrame::draw_close_button(Color fill)
{
    Painter painter(&m_owner.surface());

    auto close_button_rect = raw_close_button_rect();
    painter.fill_rect(close_button_rect, fill);

    auto close_button_bitmap_top_left = close_button_rect.top_left();
    close_button_bitmap_top_left      = { close_button_rect.left() + 6, close_button_rect.top() + 6 };
    painter.draw_bitmap(m_owner.theme()->button_bitmap(Theme::Button::CLOSE), close_button_bitmap_top_left);

    m_owner.set_invalidated(true);
}

void WindowFrame::draw_maximize_button(Color fill)
{
    Painter painter(&m_owner.surface());

    auto maximize_button_rect = raw_maximize_button_rect();
    painter.fill_rect(maximize_button_rect, fill);

    auto close_button_bitmap_top_left = maximize_button_rect.top_left();
    close_button_bitmap_top_left      = { maximize_button_rect.left() + 6, maximize_button_rect.top() + 6 };
    painter.draw_bitmap(m_owner.theme()->button_bitmap(Theme::Button::MAXIMIZE), close_button_bitmap_top_left);

    m_owner.set_invalidated(true);
}

void WindowFrame::draw_minimize_button(Color fill)
{
    Painter painter(&m_owner.surface());

    auto minimize_button_rect = raw_minimize_button_rect();
    painter.fill_rect(minimize_button_rect, fill);

    auto close_button_bitmap_top_left = minimize_button_rect.top_left();
    close_button_bitmap_top_left      = { minimize_button_rect.left() + 6, minimize_button_rect.top() + 6 };
    painter.draw_bitmap(m_owner.theme()->button_bitmap(Theme::Button::MINIMIZE), close_button_bitmap_top_left);

    m_owner.set_invalidated(true);
}

Rect WindowFrame::rect() const
{
    if (!m_owner.has_frame())
        return m_owner.window_rect();

    auto final_rect = m_owner.window_rect();

    final_rect.set_height(final_rect.height() + m_owner.theme()->upper_window_frame_height());

    return final_rect;
}

Rect WindowFrame::view_rect() const
{
    if (!m_owner.has_frame())
        return m_owner.window_rect();

    return m_owner.window_rect().translated(0, m_owner.theme()->upper_window_frame_height());
}

Rect WindowFrame::draggable_rect() const
{
    auto& theme = m_owner.theme();

    auto final_rect = rect().translated(m_owner.location());
    final_rect.set_height(theme->upper_window_frame_height());
    final_rect.set_width(final_rect.width() - theme->width_for_button(Theme::Button::CLOSE)
                         - theme->width_for_button(Theme::Button::MAXIMIZE)
                         - theme->width_for_button(Theme::Button::MINIMIZE));

    return final_rect;
}

Rect WindowFrame::raw_draggable_rect() const
{
    auto& theme = m_owner.theme();

    auto final_rect = rect();
    final_rect.set_height(theme->upper_window_frame_height());
    final_rect.set_width(final_rect.width() - theme->width_for_button(Theme::Button::CLOSE)
                         - theme->width_for_button(Theme::Button::MAXIMIZE)
                         - theme->width_for_button(Theme::Button::MINIMIZE));
    return final_rect;
}

Rect WindowFrame::close_button_rect() const
{
    auto& theme = m_owner.theme();

    auto final_rect = rect().translated(m_owner.location());

    final_rect.set_top_left_x(final_rect.right() - theme->width_for_button(Theme::Button::CLOSE));
    final_rect.set_height(theme->upper_window_frame_height());
    final_rect.set_width(theme->width_for_button(Theme::Button::CLOSE));

    return final_rect;
}

Rect WindowFrame::maximize_button_rect() const
{
    auto& theme = m_owner.theme();

    auto final_rect = rect().translated(m_owner.location());

    final_rect.set_top_left_x(final_rect.right() - theme->width_for_button(Theme::Button::MAXIMIZE)
                              - theme->width_for_button(Theme::Button::CLOSE));
    final_rect.set_height(theme->upper_window_frame_height());
    final_rect.set_width(theme->width_for_button(Theme::Button::MAXIMIZE));

    return final_rect;
}

Rect WindowFrame::minimize_button_rect() const
{
    auto& theme = m_owner.theme();

    auto final_rect = rect().translated(m_owner.location());

    final_rect.set_top_left_x(final_rect.right() - theme->width_for_button(Theme::Button::MINIMIZE)
                              - theme->width_for_button(Theme::Button::MAXIMIZE)
                              - theme->width_for_button(Theme::Button::CLOSE));
    final_rect.set_height(theme->upper_window_frame_height());
    final_rect.set_width(theme->width_for_button(Theme::Button::MINIMIZE));

    return final_rect;
}

Rect WindowFrame::raw_close_button_rect() const
{
    auto& theme = m_owner.theme();

    return { raw_draggable_rect().right() + theme->width_for_button(Theme::Button::MINIMIZE)
                 + theme->width_for_button(Theme::Button::CLOSE),
             0,
             theme->width_for_button(Theme::Button::CLOSE),
             theme->upper_window_frame_height() };
}

Rect WindowFrame::raw_maximize_button_rect() const
{
    auto& theme = m_owner.theme();

    return { raw_draggable_rect().right() + theme->width_for_button(Theme::Button::MAXIMIZE),
             0,
             theme->width_for_button(Theme::Button::CLOSE),
             theme->upper_window_frame_height() };
}

Rect WindowFrame::raw_minimize_button_rect() const
{
    auto& theme = m_owner.theme();

    return { raw_draggable_rect().right(),
             0,
             theme->width_for_button(Theme::Button::MINIMIZE),
             theme->upper_window_frame_height() };
}
}
