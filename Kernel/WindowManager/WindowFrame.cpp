#include "WindowFrame.h"
#include "Compositor.h"
#include "Painter.h"
#include "Window.h"

namespace kernel {

WindowFrame::WindowFrame(Window& owner)
    : m_owner(owner)
{
}

void WindowFrame::paint()
{
    if (!m_owner.has_frame())
        return;

    Painter painter(&m_owner.surface());

    auto& theme = m_owner.theme();

    painter.fill_rect(raw_rect_for_frame(Frame::TOP), theme.window_frame_color());
    painter.fill_rect(raw_rect_for_frame(Frame::LEFT), theme.window_frame_color());
    painter.fill_rect(raw_rect_for_frame(Frame::RIGHT), theme.window_frame_color());
    painter.fill_rect(raw_rect_for_frame(Frame::BOTTOM), theme.window_frame_color());

    draw_button(Button::CLOSE, ButtonState::RELEASED);
    draw_button(Button::MAXIMIZE, ButtonState::RELEASED);
    draw_button(Button::MINIMIZE, ButtonState::RELEASED);
    draw_title();
}

void WindowFrame::on_button_state_changed(Button button, ButtonState state)
{
    draw_button(button, state);
}

void WindowFrame::draw_button(Button button, ButtonState state)
{
    Painter painter(&m_owner.surface());

    auto& theme = m_owner.theme();
    auto button_rect = raw_rect_for_button(button);

    painter.fill_rect(button_rect, theme.color_for_button_state(button, state));

    // TODO: unhardcode padding
    painter.draw_bitmap(theme.button_bitmap(button), { button_rect.left() + 6, button_rect.top() + 6 });
    m_owner.invalidate_rect(button_rect);
}

void WindowFrame::draw_title()
{
    auto& title = m_owner.title();

    if (title.empty())
        return;

    Painter painter(&m_owner.surface());

    static constexpr ssize_t y_offset = 1;

    ssize_t x_offset = 2;
    Rect title_rect = raw_draggable_rect();
    title_rect.set_top_left({ x_offset, y_offset });
    title_rect.set_width(title_rect.width() - x_offset);
    title_rect.set_height(title_rect.height() - y_offset);

    size_t max_title_length = title_rect.width() / Painter::font_width;

    bool title_too_long = max_title_length < m_owner.title().size();

    painter.set_clip_rect(title_rect);

    auto write_title_text = [&](StringView text) {
        for (char c : text) {
            painter.draw_char({ x_offset, y_offset }, c, Color::white(), m_owner.theme().window_frame_color());
            x_offset += Painter::font_width;
        }
    };

    size_t chars_to_write = title.size();
    if (title_too_long) {
        chars_to_write = max_title_length - 3;
    }

    write_title_text(StringView(m_owner.title().c_string(), chars_to_write));

    if (title_too_long) {
        write_title_text("..."_sv);
    }

    painter.reset_clip_rect();

    m_owner.invalidate_rect(title_rect);
}

Rect WindowFrame::rect() const
{
    if (!m_owner.has_frame())
        return m_owner.window_rect();

    auto final_rect = m_owner.window_rect();

    auto& theme = m_owner.theme();

    final_rect.set_height(final_rect.height() + theme.upper_window_frame_height()
        + theme.bottom_width_frame_height());
    final_rect.set_width(final_rect.width() + (theme.side_window_frame_width() * 2));

    return final_rect;
}

Rect WindowFrame::view_rect() const
{
    if (!m_owner.has_frame())
        return m_owner.window_rect();

    auto& theme = m_owner.theme();

    auto window_rect = m_owner.window_rect();

    window_rect.set_top_left_x(theme.side_window_frame_width());
    window_rect.set_top_left_y(theme.upper_window_frame_height());

    return window_rect;
}

Rect WindowFrame::raw_rect_for_frame(Frame frame) const
{
    auto& theme = m_owner.theme();
    auto frame_rect = rect();
    auto view_rect = m_owner.view_rect();

    switch (frame) {
    case Frame::TOP:
        return { frame_rect.top_left(), frame_rect.width(), theme.upper_window_frame_height() };
    case Frame::LEFT:
        return { 0, theme.upper_window_frame_height(), theme.side_window_frame_width(), view_rect.height() };
    case Frame::RIGHT:
        return {
            theme.side_window_frame_width() + view_rect.width(),
            theme.upper_window_frame_height(),
            theme.side_window_frame_width(),
            view_rect.height()
        };
    case Frame::BOTTOM:
        return {
            frame_rect.left(),
            frame_rect.height() - theme.bottom_width_frame_height(),
            frame_rect.width(),
            theme.bottom_width_frame_height()
        };
    default:
        ASSERT_NEVER_REACHED();
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
    final_rect.set_height(theme.upper_window_frame_height());
    final_rect.set_width(final_rect.width() - theme.width_for_button(Button::CLOSE)
        - theme.width_for_button(Button::MAXIMIZE) - theme.width_for_button(Button::MINIMIZE));
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
    Rect final_rect = {
        raw_draggable_rect().right(),
        0,
        theme.width_for_button(Button::MINIMIZE),
        theme.upper_window_frame_height()
    };

    if (button == Button::MINIMIZE)
        return final_rect;

    // final_rect == MAXIMIZE
    final_rect.set_top_left_x(final_rect.left() + theme.width_for_button(Button::MINIMIZE));
    final_rect.set_width(theme.width_for_button(Button::MAXIMIZE));

    if (button == Button::MAXIMIZE)
        return final_rect;

    // final_rect == CLOSE
    final_rect.set_top_left_x(final_rect.left() + theme.width_for_button(Button::MAXIMIZE));
    final_rect.set_width(theme.width_for_button(Button::CLOSE));

    if (button == Button::CLOSE)
        return final_rect;

    ASSERT_NEVER_REACHED();
}
}
