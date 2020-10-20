#include "WindowFrame.h"
#include "Painter.h"
#include "Window.h"

namespace kernel {

// clang-format off
const u8 WindowFrame::s_close_button_bitmap_data[] = {
    0b10000001,
    0b01000010,
    0b00100100,
    0b00011000,
    0b00011000,
    0b00100100,
    0b01000010,
    0b10000001,
};
// clang-format on

const Color WindowFrame::s_palette[2] = { Color::transparent(), Color::white() };

WindowFrame::WindowFrame(Window& owner)
    : m_owner(owner), m_close_button_bitmap(s_close_button_bitmap_data,
                                            close_button_bitmap_width,
                                            close_button_bitmap_height,
                                            Bitmap::Format::INDEXED_1_BPP,
                                            s_palette)
{
}

void WindowFrame::paint()
{
    if (!m_owner.has_frame())
        return;

    Painter painter(&m_owner.surface());

    auto frame_rect = rect();
    painter.fill_rect({ frame_rect.top_left(), frame_rect.width(), upper_frame_height }, default_frame_color);
    on_close_button_released();
}

void WindowFrame::on_close_button_hovered()
{
    draw_close_button(Color::red());
}

void WindowFrame::on_close_button_released()
{
    draw_close_button(default_frame_color);
}

void WindowFrame::draw_close_button(Color fill)
{
    Painter painter(&m_owner.surface());

    auto close_button_rect = raw_close_button_rect();
    painter.fill_rect(close_button_rect, fill);

    auto close_button_bitmap_top_left = close_button_rect.top_left();
    close_button_bitmap_top_left      = { close_button_rect.left() + 6, close_button_rect.top() + 6 };
    painter.draw_bitmap(m_close_button_bitmap, close_button_bitmap_top_left);

    m_owner.set_invalidated(true);
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

Rect WindowFrame::draggable_rect() const
{
    auto final_rect = rect().translated(m_owner.location());
    final_rect.set_height(upper_frame_height);
    final_rect.set_width(final_rect.width() - close_button_width - minimize_button_width);

    return final_rect;
}

Rect WindowFrame::close_button_rect() const
{
    auto final_rect = rect().translated(m_owner.location());

    final_rect.set_top_left_x(final_rect.right() - close_button_width);
    final_rect.set_height(upper_frame_height);
    final_rect.set_width(close_button_width);

    return final_rect;
}

Rect WindowFrame::raw_close_button_rect() const
{
    auto frame_rect = rect();
    return { frame_rect.right() - close_button_width, 0, close_button_width, upper_frame_height };
}

Rect WindowFrame::minimize_button_rect() const
{
    return { 0, 0, 0, 0 };
}
}
