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

    painter.fill_rect(raw_rect_for_frame(Frame::TOP), theme->window_frame_color());
    painter.fill_rect(raw_rect_for_frame(Frame::LEFT), theme->window_frame_color());
    painter.fill_rect(raw_rect_for_frame(Frame::RIGHT), theme->window_frame_color());
    painter.fill_rect(raw_rect_for_frame(Frame::BOTTOM), theme->window_frame_color());

    draw_button(Button::CLOSE, ButtonState::RELEASED);
    draw_button(Button::MAXIMIZE, ButtonState::RELEASED);
    draw_button(Button::MINIMIZE, ButtonState::RELEASED);
}

void WindowFrame::on_button_state_changed(Button button, ButtonState state)
{
    draw_button(button, state);
}

void WindowFrame::draw_button(Button button, ButtonState state)
{
    Painter painter(&m_owner.surface());

    auto& theme       = m_owner.theme();
    auto  button_rect = raw_rect_for_button(button);

    painter.fill_rect(button_rect, theme->color_for_button_state(button, state));

    // TODO: unhardcode padding
    painter.draw_bitmap(theme->button_bitmap(button), { button_rect.left() + 6, button_rect.top() + 6 });

    m_owner.set_invalidated(true);
}

Rect WindowFrame::rect() const
{
    if (!m_owner.has_frame())
        return m_owner.window_rect();

    auto final_rect = m_owner.window_rect();

    final_rect.set_height(final_rect.height() + m_owner.theme()->upper_window_frame_height()
                          + m_owner.theme()->bottom_width_frame_height());
    final_rect.set_width(final_rect.width() + (m_owner.theme()->side_window_frame_width() * 2));

    return final_rect;
}

Rect WindowFrame::view_rect() const
{
    if (!m_owner.has_frame())
        return m_owner.window_rect();

    auto& theme = m_owner.theme();

    auto window_rect = m_owner.window_rect();

    window_rect.set_top_left_x(theme->side_window_frame_width());
    window_rect.set_top_left_y(theme->upper_window_frame_height());

    return window_rect;
}

Rect WindowFrame::raw_rect_for_frame(Frame frame) const
{
    auto& theme      = m_owner.theme();
    auto  frame_rect = rect();
    auto  view_rect  = m_owner.view_rect();

    switch (frame) {
    case Frame::TOP: return { frame_rect.top_left(), frame_rect.width(), theme->upper_window_frame_height() };
    case Frame::LEFT:
        return { 0, theme->upper_window_frame_height(), theme->side_window_frame_width(), view_rect.height() };
    case Frame::RIGHT:
        return { theme->side_window_frame_width() + view_rect.width(),
                 theme->upper_window_frame_height(),
                 theme->side_window_frame_width(),
                 view_rect.height() };
    case Frame::BOTTOM:
        return { frame_rect.left(),
                 frame_rect.height() - theme->bottom_width_frame_height(),
                 frame_rect.width(),
                 theme->bottom_width_frame_height() };
    default: ASSERT_NEVER_REACHED();
    }
}

Rect WindowFrame::draggable_rect() const
{
    return raw_draggable_rect().translated(m_owner.location());
}

Rect WindowFrame::raw_draggable_rect() const
{
    auto& theme = m_owner.theme();

    auto final_rect = rect();
    final_rect.set_height(theme->upper_window_frame_height());
    final_rect.set_width(final_rect.width() - theme->width_for_button(Button::CLOSE)
                         - theme->width_for_button(Button::MAXIMIZE) - theme->width_for_button(Button::MINIMIZE));
    return final_rect;
}

Rect WindowFrame::rect_for_button(Button button) const
{
    return raw_rect_for_button(button).translated(m_owner.location());
}

Rect WindowFrame::raw_rect_for_button(Button button) const
{
    auto& theme = m_owner.theme();

    // final_rect == MINIMIZE
    Rect final_rect = { raw_draggable_rect().right(),
                        0,
                        theme->width_for_button(Button::MINIMIZE),
                        theme->upper_window_frame_height() };

    if (button == Button::MINIMIZE)
        return final_rect;

    // final_rect == MAXIMIZE
    final_rect.set_top_left_x(final_rect.left() + theme->width_for_button(Button::MINIMIZE));
    final_rect.set_width(theme->width_for_button(Button::MAXIMIZE));

    if (button == Button::MAXIMIZE)
        return final_rect;

    // final_rect == CLOSE
    final_rect.set_top_left_x(final_rect.left() + theme->width_for_button(Button::MAXIMIZE));
    final_rect.set_width(theme->width_for_button(Button::CLOSE));

    if (button == Button::CLOSE)
        return final_rect;

    ASSERT_NEVER_REACHED();
}
}
