#include "DemoTTY.h"
#include "EventManager.h"
#include "Multitasking/Process.h"
#include "WindowManager/WindowManager.h"

namespace kernel {

DemoTTY* DemoTTY::s_instance;

void DemoTTY::initialize()
{
    ASSERT(s_instance == nullptr);

    s_instance = new DemoTTY();
    Process::create_supervisor(&DemoTTY::run);
}

DemoTTY::DemoTTY()
    : m_window(Window::create(*Thread::current(), window_rect, WindowManager::the().active_theme(), "Terminal"_sv))
    , m_painter(&m_window->surface())
    , m_view_rect(m_window->view_rect())
{
    m_painter.fill_rect(m_view_rect, background_color);
    m_window->invalidate_part_of_view_rect({ 0, 0, m_view_rect.width(), m_view_rect.height() });

    m_view_rect.set_top_left_x(m_view_rect.left() + padding_x);
    m_view_rect.set_top_left_y(m_view_rect.top() + padding_y);
    m_view_rect.set_width(m_view_rect.width() - padding_x);
    m_view_rect.set_height(m_view_rect.height() - padding_y);

    m_width_in_char = m_view_rect.width() / Painter::font_width;
    m_height_in_char = m_view_rect.height() / Painter::font_height;

    write(command_prompt);
    draw_cursor();
    m_prompt_y = m_y_offset;
}

void DemoTTY::run()
{
    while (true) {
        the().tick();
    }
}

void DemoTTY::tick()
{
    static constexpr size_t cursor_blink_timeout_ns = 500 * Time::nanoseconds_in_millisecond;
    bool cursor_should_blink = m_last_cursor_blink_time + cursor_blink_timeout_ns < Timer::the().nanoseconds_since_boot();

    if (cursor_should_blink) {
        if (m_cursor_is_shown) {
            m_x_offset += 1;
            clear_characters(1, false);
            m_cursor_is_shown = false;
            m_last_cursor_blink_time = Timer::the().nanoseconds_since_boot();
        } else {
            draw_cursor();
        }
    }

    LockGuard lock_guard(m_window->event_queue_lock());

    for (auto& event : m_window->event_queue()) {
        switch (event.type) {
        case Event::Type::KEY_STATE: {
            if (event.vk_state.state == VKState::RELEASED)
                break;

            if (event.vk_state.vkey == VK::BACKSPACE) {
                clear_characters(1);
                m_current_command.pop_back();
                break;
            }

            if (event.vk_state.vkey == VK::ENTER) {
                execute_command();
                break;
            }
            break;
        }
        case Event::Type::CHAR_TYPED: {
            auto ascii_char = static_cast<char>(event.char_typed.character);
            m_current_command.append(ascii_char);
            write({ &ascii_char, 1 });
            draw_cursor();
        }
        default:
            break;
        }
    }

    m_window->event_queue().clear();
}

void DemoTTY::clear_screen()
{
    auto height_to_fill = Painter::font_height * (m_y_offset + 1);

    Rect clear_rect = m_view_rect;
    clear_rect.set_height(height_to_fill);

    m_painter.fill_rect(clear_rect, background_color);
    m_window->invalidate_part_of_view_rect({ padding_x, padding_y, m_view_rect.width(), height_to_fill });

    m_y_offset = 0;
    m_x_offset = 0;
}

void DemoTTY::execute_command()
{
    if (m_cursor_is_shown) {
        m_x_offset++;
        clear_characters(1, false);
    }

    if (m_current_command == "clear"_sv) {
        clear_screen();
    } else if (m_current_command == "uptime"_sv) {
        write("\nUptime: ");
        char buffer[21];
        auto seconds_since_boot = Timer::the().nanoseconds_since_boot() / Time::nanoseconds_in_second;
        write({ buffer, to_string(seconds_since_boot, buffer, 21) });
        write(" seconds\n");
    } else if (m_current_command == "kheap"_sv) {
        StackStringBuilder string;

        string << "\nKHeap stats:\n";

        auto stats = HeapAllocator::stats();
        auto used_bytes = stats.total_bytes - stats.free_bytes;
        auto usage = (100 * used_bytes) / stats.total_bytes;

        string << "Total bytes: " << stats.total_bytes << " (~" << stats.total_bytes / KB << " KB) " << '\n';
        string << "Free bytes: " << stats.free_bytes << " (~" << stats.free_bytes / KB << " KB) " << '\n';
        string << "Used bytes: " << used_bytes << " (~" << used_bytes / KB << " KB) " << '\n';
        string << "Usage: " << usage << "%\n";
        string << "Heap blocks: " << stats.heap_blocks << '\n';
        string << "Calls to allocate: " << stats.calls_to_allocate << '\n';
        string << "Calls to free: " << stats.calls_to_free << '\n';

        write(string.as_view());
    } else if (m_current_command == "help"_sv) {
        write("\nWelcome to UltraOS demo terminal.\n"_sv);
        write("Here's a few things you can do:\n"_sv);
        write("uptime - get current uptime\n"_sv);
        write("kheap - get current kernel heap usage stats\n"_sv);
        write("clear - clear the terminal screen\n"_sv);
    } else if (m_current_command.empty()) {
        write("\n");
    } else {
        write("\nUnknown command: "_sv);
        write(m_current_command.to_view());
        write("\n");
    }

    m_current_command.clear();
    write(command_prompt);
    draw_cursor();
    m_prompt_y = m_y_offset;
}

void DemoTTY::clear_characters(size_t count, bool should_draw_cursor)
{
    auto decrement_x = [&]() {
        m_x_offset--;
        if (m_x_offset == 0) {
            m_y_offset -= 1;
            m_x_offset = m_width_in_char;
        }
    };

    for (size_t i = 0; i < count; ++i) {
        if (m_y_offset == m_prompt_y && (m_x_offset <= static_cast<ssize_t>(command_prompt.size())))
            break;

        if (m_cursor_is_shown) {
            write(" "_sv);
            decrement_x();
        }

        decrement_x();
        write(" "_sv);
        decrement_x();
    }

    if (should_draw_cursor)
        draw_cursor();
}

void DemoTTY::scroll(Direction direction)
{
    ASSERT(direction == Direction::UP);

    Rect scrolled_rect = m_view_rect;
    scrolled_rect.set_top_left_y(m_view_rect.top() + Painter::font_height);
    scrolled_rect.set_height(scrolled_rect.height() - Painter::font_height);

    m_painter.blit(m_view_rect.top_left(), m_window->surface(), scrolled_rect);
    m_painter.fill_rect({ m_view_rect.left(), m_view_rect.top() + scrolled_rect.height(), m_width_in_char * Painter::font_width, Painter::font_height }, background_color);
    m_window->invalidate_part_of_view_rect({ padding_x, padding_y, m_view_rect.width(), m_view_rect.height() });
}

void DemoTTY::draw_cursor()
{
    if (m_x_offset == m_width_in_char) {
        m_x_offset = 0;
        m_y_offset += 1;
    }

    if (m_y_offset == m_height_in_char) {
        scroll(Direction::UP);
        m_y_offset -= 1;
        m_prompt_y -= 1;
    }

    Point top_left = { m_x_offset * Painter::font_width + 1, m_y_offset * Painter::font_height };
    Rect to_fill = { top_left.moved_by(m_view_rect.top_left()), 1, Painter::font_height };

    m_painter.fill_rect(to_fill, Color::white());
    m_window->invalidate_part_of_view_rect({ top_left.moved_by({ padding_x, padding_y }), 1, Painter::font_height });

    m_last_cursor_blink_time = Timer::the().nanoseconds_since_boot();
    m_cursor_is_shown = true;
}

void DemoTTY::write(StringView string)
{
    for (char c : string) {
        if (c == '\n') {
            m_y_offset += 1;
            m_x_offset = 0;
            continue;
        }

        if (m_x_offset == m_width_in_char) {
            m_x_offset = 0;
            m_y_offset += 1;
        }

        if (m_y_offset == m_height_in_char) {
            scroll(Direction::UP);
            m_y_offset -= 1;
            m_prompt_y -= 1;
        }

        Point top_left = { m_x_offset * Painter::font_width, m_y_offset * Painter::font_height };

        m_painter.draw_char(top_left.moved_by(m_view_rect.top_left()), c, Color::white(), background_color);
        m_window->invalidate_part_of_view_rect({ top_left.moved_by({ padding_x, padding_y }), Painter::font_width, Painter::font_height });

        m_x_offset += 1;
    }
}
}
