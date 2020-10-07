#include "Common/Logger.h"
#include "Common/Macros.h"
#include "Common/String.h"

#include "Drivers/Video/VideoDevice.h"
#include "Interrupts/IPICommunicator.h"
#include "Interrupts/InterruptController.h"
#include "Memory/MemoryManager.h"
#include "WindowManager/Painter.h"

extern "C" void __cxa_pure_virtual()
{
    kernel::error() << "A pure virtual call!";
    hang();
}

using global_constructor_t = void (*)();
global_constructor_t global_constructors_begin;
global_constructor_t global_constructors_end;

constexpr size_t  magic_length = 13;
extern const char magic_string[magic_length];

namespace kernel::runtime {
// clang-format off
void ensure_loaded_correctly()
{
    // Global constructors are not yet called so we cannot use log()
    // instead, just create a temporary logger here
    E9LogSink e9s;

    if (magic_string[0]  == 'M' &&
        magic_string[1]  == 'A' &&
        magic_string[2]  == 'G' &&
        magic_string[3]  == 'I' &&
        magic_string[4]  == 'C' &&
        magic_string[5]  == '_' &&
        magic_string[6]  == 'P' &&
        magic_string[7]  == 'R' &&
        magic_string[8]  == 'E' &&
        magic_string[9]  == 'S' &&
        magic_string[10] == 'E' &&
        magic_string[11] == 'N' &&
        magic_string[12] == 'T') {
        if (e9s.is_supported())  {
            e9s.write(Logger::info_prefix);
            e9s.write("Runtime: Magic test passed!\n");
        }
    } else {
        if (e9s.is_supported())  {
            e9s.write(Logger::error_prefix);
            e9s.write("Runtime: Magic test failed!\n");
        }
        hang();
    }
}

void init_global_objects()
{
    for (global_constructor_t* ctor = &global_constructors_begin; ctor < &global_constructors_end; ++ctor)
        (*ctor)();
}
// clang-format on

void on_assertion_failed(const char* message, const char* file, const char* function, u32 line)
{
    static constexpr auto assertion_failed = "Assertion failed!"_sv;
    static constexpr auto expression       = "\n------> Expression : "_sv;
    static constexpr auto function_str     = "\n------> Function   : "_sv;
    static constexpr auto file_str         = "\n------> File       : "_sv;

    // We don't want to use the heap here as it might be corrupted
    StackStringBuilder<512> formatted_message;
    formatted_message << assertion_failed << expression << message << function_str << function << file_str << file
                      << ':' << line;

    panic(formatted_message.data());
}

size_t dump_backtrace(ptr_t* into, size_t max_depth)
{
    Address base_pointer;

#ifdef ULTRA_32
    asm("mov %%ebp, %0" : "=a"(base_pointer));
#elif defined(ULTRA_64)
    asm("mov %%rbp, %0" : "=a"(base_pointer));
#endif

    size_t current_depth = 0;

    auto is_address_valid = [](Address adr) -> bool {
        return adr > MemoryManager::kernel_reserved_base && adr < MemoryManager::kernel_end_address;
    };

    while (--max_depth && is_address_valid(base_pointer)) {
        into[current_depth++] = base_pointer.as_pointer<ptr_t>()[1];
        base_pointer          = base_pointer.as_pointer<ptr_t>()[0];
    }

    return current_depth ? current_depth - 1 : 0;
}

[[noreturn]] void panic(const char* reason)
{
    static constexpr auto  panic_message = "KERNEL PANIC!"_sv;
    static constexpr Color font_color    = Color::white();
    static constexpr Color screen_color  = Color::blue();

    error() << panic_message << "\n";

    if (InterruptController::is_initialized())
        IPICommunicator::hang_all_cores();

    if (!VideoDevice::is_ready())
        hang();

    auto& surface = VideoDevice::the().surface();
    Rect  surface_rect(0, 0, surface.width(), surface.height());
    auto  center = surface_rect.center();
    Rect  exception_rect(0, 0, center.x(), center.y());
    Point offset    = exception_rect.center();
    auto  initial_x = offset.x();

    Painter p(&surface);
    p.fill_rect(surface_rect, screen_color);

    Point panic_offset(offset.x() + offset.x() / 2 + (panic_message.size() / 2 * Painter::font_width), offset.y() - 40);

    for (char c: panic_message) {
        p.draw_char(panic_offset, c, font_color, Color::transparent());
        panic_offset.first() += Painter::font_width;
    }

    Logger bt_logger(false);

    auto write_string = [&](StringView string) {
        bt_logger << string;
        for (char c: StringView(string)) {
            if (c == '\n') {
                offset.second() += Painter::font_height;
                offset.first() = initial_x;

                continue;
            }

            p.draw_char(offset, c, font_color, Color::transparent());
            offset.first() += Painter::font_width;
        }
    };

    write_string(reason);

    offset.second() += Painter::font_height * 2;
    offset.first() = initial_x;

    bt_logger << "\n\n"_sv;
    write_string("Backtrace:\n"_sv);

    ptr_t backtrace[8] {};
    auto  depth = dump_backtrace(backtrace, 8);

    if (depth < 2) {
        write_string("No backtrace available (possible stack corruption)"_sv);
        hang();
    }

    static constexpr auto frame_message = "Frame "_sv;

    char number[20];

    for (size_t current_depth = 1; current_depth < depth; ++current_depth) {
        write_string(frame_message);

        to_string(current_depth, number, 20);
        write_string(number);
        write_string(": "_sv);
        to_hex_string(backtrace[current_depth], number, 20);
        write_string(number);
        write_string("\n"_sv);
    }

    hang();
}
}
