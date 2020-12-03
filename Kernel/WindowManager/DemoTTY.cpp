#include "DemoTTY.h"
#include "Drivers/Video/VideoDevice.h"
#include "EventManager.h"
#include "Memory/MemoryManager.h"
#include "Multitasking/Process.h"
#include "WindowManager/WindowManager.h"

namespace kernel {

DemoTTY* DemoTTY::s_instance;

void DemoTTY::initialize()
{
    ASSERT(s_instance == nullptr);

    s_instance = new DemoTTY();
    Process::create_supervisor(&DemoTTY::run, 4 * MB);
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

    m_scrollback_buffer.emplace().expand_to(m_width_in_char);
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

    if (!is_scrolled()) {
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
    } else {
        m_cursor_is_shown = false;
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
            break;
        }
        case Event::Type::MOUSE_SCROLL: {
            if (event.mouse_scroll.delta > 0)
                scroll(Direction::UP, 3);
            else
                scroll(Direction::DOWN, 3);
            break;
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
    m_scrollback_top_y = 0;
    m_scrollback_current_y = 0;
    m_prescroll_top_y = 0;
    m_scrollback_buffer.clear();
    m_scrollback_buffer.emplace().expand_to(m_width_in_char);
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
    } else if (m_current_command == "memory-map"_sv) {
        auto& map = MemoryManager::the().memory_map();
        write("\nPhysical memory map: ");

        for (auto& entry : map) {
            write("\n"_sv);
            StackStringBuilder<128> range_str;
            range_str << format::as_hex << "base: " << entry.base_address;
            range_str << format::as_dec << "\nlength: " << entry.length / KB << " KB";
            range_str << "\ntype: " << entry.type_as_string();
            write(range_str.as_view());
            write("\n"_sv);
        }
    } else if (m_current_command == "video-mode"_sv) {
        write("\nVideo mode information:\n"_sv);

        auto info = VideoDevice::the().mode();
        StackStringBuilder<128> info_string;
        info_string << "Resolution: " << info.width << "x" << info.height << " @ " << info.bpp << " bpp";
        info_string << "\nDevice: \"" << VideoDevice::the().name() << '\"';
        write(info_string.as_view());
        write("\n"_sv);
    } else if (m_current_command == "cpu"_sv) {
        write("\nCPU information:\n"_sv);

        StackStringBuilder<128> info_string;
        info_string << "Supports SMP: " << CPU::supports_smp();
        info_string << "\nCores: " << CPU::processor_count();
        info_string << "\nAlive cores: " << CPU::alive_processor_count();
        write(info_string.as_view());
        write("\n"_sv);
    } else if (m_current_command == "help"_sv) {
        write("\nWelcome to UltraOS demo terminal.\n"_sv);
        write("Here's a few things you can do:\n"_sv);
        write("uptime - get current uptime\n"_sv);
        write("kheap - get current kernel heap usage stats\n"_sv);
        write("memory-map - physical RAM memory map as reported by BIOS\n"_sv);
        write("vmdump - get kernel address space virtual memory dump\n"_sv);
        write("video-mode - get current video mode information\n"_sv);
        write("cpu - get CPU information\n"_sv);
        write("clear - clear the terminal screen\n"_sv);
    } else if (m_current_command == "vmdump"_sv) {
        write("\nVirtual memory dump:\n");
        write(AddressSpace::of_kernel().allocator().debug_dump().to_view());
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
    if (is_scrolled())
        scroll_to_bottom();

    auto decrement_x = [&]() {
        m_x_offset--;
        if (m_x_offset == 0) {
            m_y_offset -= 1;
            m_scrollback_current_y -= 1;
            m_x_offset = m_width_in_char;
        }
    };

    for (size_t i = 0; i < count; ++i) {
        if (m_y_offset == m_prompt_y && (m_x_offset <= static_cast<ssize_t>(command_prompt.size())))
            break;

        if (m_cursor_is_shown && should_draw_cursor) {
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

void DemoTTY::redraw_screen()
{
    auto new_line_rect = m_view_rect;
    new_line_rect.set_height(Painter::font_height);

    for (ssize_t i = 0; i < m_height_in_char; ++i) {
        auto scrollback_index = m_scrollback_top_y + i;
        if (static_cast<ssize_t>(m_scrollback_buffer.size()) > scrollback_index) {
            auto top_left = new_line_rect.top_left();
            for (char c : m_scrollback_buffer[scrollback_index]) {
                if (c == 0)
                    c = ' ';
                m_painter.draw_char(top_left, c, Color::white(), background_color);
                top_left.move_by(Painter::font_width, 0);
            }
        } else
            m_painter.fill_rect(new_line_rect, background_color);

        new_line_rect.translate(0, Painter::font_height);
    }

    m_window->invalidate_part_of_view_rect({ padding_x, padding_y, m_view_rect.width(), m_view_rect.height() });
}

void DemoTTY::scroll(Direction direction, ssize_t lines, bool force)
{
    if (direction == Direction::UP) {
        if (m_scrollback_top_y == m_scrollback_current_y || (is_current_line_visible() && !force))
            return;

        lines = min(lines, m_scrollback_current_y);

        // we have to basically redraw the entire screen
        if (lines >= m_height_in_char) {
            m_scrollback_top_y += lines;
            redraw_screen();
            return;
        }

        if (!force) {
            auto final_y = m_scrollback_top_y + m_height_in_char + lines - 1;
            if (final_y > m_scrollback_current_y) {
                lines -= final_y - m_scrollback_current_y;
            }

            if (lines <= 0)
                return;
        }

        if (!is_scrolled())
            m_prescroll_top_y = m_scrollback_top_y;

        auto height_to_scroll = Painter::font_height * lines;
        Rect scrolled_rect = m_view_rect;
        scrolled_rect.set_top_left_y(m_view_rect.top() + height_to_scroll);
        scrolled_rect.set_height(scrolled_rect.height() - height_to_scroll);

        m_painter.blit(m_view_rect.top_left(), m_window->surface(), scrolled_rect);

        m_scrollback_top_y += lines;

        auto lines_filled = m_height_in_char - lines;

        Rect new_line_rect = { m_view_rect.left(),
            m_view_rect.top() + Painter::font_height * lines_filled,
            Painter::font_width * m_width_in_char,
            Painter::font_height };

        for (ssize_t i = 0; i < lines; ++i) {
            auto scrollback_index = m_scrollback_top_y + lines_filled + i;
            if (static_cast<ssize_t>(m_scrollback_buffer.size()) > scrollback_index) {
                auto top_left = new_line_rect.top_left();
                for (char c : m_scrollback_buffer[scrollback_index]) {
                    if (c == 0)
                        c = ' ';
                    m_painter.draw_char(top_left, c, Color::white(), background_color);
                    top_left.move_by(Painter::font_width, 0);
                }
            } else
                m_painter.fill_rect(new_line_rect, background_color);

            new_line_rect.translate(0, Painter::font_height);
        }

        m_window->invalidate_part_of_view_rect({ padding_x, padding_y, m_view_rect.width(), m_view_rect.height() });
    } else {
        ASSERT(direction == Direction::DOWN);

        if (m_scrollback_top_y == 0)
            return;

        ASSERT(lines < m_height_in_char);

        lines = min(lines, m_scrollback_top_y);

        if (!is_scrolled())
            m_prescroll_top_y = m_scrollback_top_y;

        m_scrollback_top_y -= lines;

        Rect scrolled_rect = m_view_rect;
        scrolled_rect.set_height(Painter::font_height * (m_height_in_char - lines));

        u8* surface_data = new u8[scrolled_rect.width() * scrolled_rect.height() * 4];
        MutableBitmap temp_surface(surface_data, scrolled_rect.width(), scrolled_rect.height(), MutableBitmap::Format::RGBA_32_BPP);
        Painter temp_painter(&temp_surface);

        temp_painter.blit({ 0, 0 }, m_window->surface(), scrolled_rect);
        m_painter.blit({ m_view_rect.left(), m_view_rect.top() + Painter::font_height * lines }, temp_surface, { 0, 0, scrolled_rect.width(), scrolled_rect.height() });
        delete[] surface_data;

        Point initial_top_left = { m_view_rect.top_left() };

        for (ssize_t i = 0; i < lines; ++i) {
            auto top_left = initial_top_left;
            for (char c : m_scrollback_buffer[m_scrollback_top_y + i]) {
                if (c == 0)
                    c = ' ';
                m_painter.draw_char(top_left, c, Color::white(), background_color);
                top_left.move_by(Painter::font_width, 0);
            }
            initial_top_left.move_by(0, Painter::font_height);
        }
        m_window->invalidate_part_of_view_rect({ padding_x, padding_y, m_view_rect.width(), m_view_rect.height() });
    }
}

void DemoTTY::draw_cursor()
{
    // expects the currently active line to be visible
    ASSERT(is_current_line_visible());
    ASSERT(m_x_offset <= m_width_in_char && m_y_offset <= m_height_in_char);

    if (m_x_offset == m_width_in_char)
        new_line();

    Point top_left = { m_x_offset * Painter::font_width + 1, m_y_offset * Painter::font_height };
    Rect to_fill = { top_left.moved_by(m_view_rect.top_left()), 1, Painter::font_height };

    m_painter.fill_rect(to_fill, Color::white());
    m_window->invalidate_part_of_view_rect({ top_left.moved_by({ padding_x, padding_y }), 1, Painter::font_height });

    m_last_cursor_blink_time = Timer::the().nanoseconds_since_boot();
    m_cursor_is_shown = true;
}

void DemoTTY::new_line()
{
    ASSERT(m_x_offset <= m_width_in_char && m_y_offset <= m_height_in_char);

    m_y_offset += 1;
    m_x_offset = 0;
    m_scrollback_current_y += 1;

    if (m_y_offset == m_height_in_char) {
        scroll(Direction::UP, 1, true);
        m_prescroll_top_y = m_scrollback_top_y;
        m_y_offset -= 1;
        m_prompt_y -= 1;
    }

    if (m_scrollback_current_y + 1 > static_cast<ssize_t>(m_scrollback_buffer.size()))
        m_scrollback_buffer.emplace().expand_to(m_width_in_char);
}

void DemoTTY::scroll_to_bottom()
{
    scroll(Direction::UP, m_prescroll_top_y - m_scrollback_top_y);

    // I think this should always be true? I'll leave it here just in case
    ASSERT(m_scrollback_top_y == m_prescroll_top_y);
}

void DemoTTY::write(StringView string)
{
    for (char c : string) {
        if (is_scrolled())
            scroll_to_bottom();

        if (c == '\n' || m_x_offset == m_width_in_char)
            new_line();

        if (c == '\n')
            continue;

        ASSERT(static_cast<ssize_t>(m_scrollback_buffer.size()) >= m_scrollback_current_y + 1);
        ASSERT(static_cast<ssize_t>(m_scrollback_buffer.at(m_scrollback_current_y).size()) > m_x_offset);
        m_scrollback_buffer[m_scrollback_current_y][m_x_offset] = c;

        Point top_left = { m_x_offset * Painter::font_width, m_y_offset * Painter::font_height };

        m_painter.draw_char(top_left.moved_by(m_view_rect.top_left()), c, Color::white(), background_color);
        m_window->invalidate_part_of_view_rect({ top_left.moved_by({ padding_x, padding_y }), Painter::font_width, Painter::font_height });

        m_x_offset += 1;
    }
}
}
