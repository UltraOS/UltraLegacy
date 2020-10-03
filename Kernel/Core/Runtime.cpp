#include "Common/Logger.h"
#include "Common/Macros.h"
#include "Common/String.h"

#include "Drivers/Video/VideoDevice.h"
#include "Interrupts/IPICommunicator.h"
#include "Interrupts/InterruptController.h"
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


void on_assertion_failed(const char* message, const char* file, const char* function, u32 line)
{
    static constexpr auto assertion_failed = "Assertion failed!"_sv;
    static constexpr auto expression = "\n------> Expression : "_sv;
    static constexpr auto function_str = "\n------> Function   : "_sv;
    static constexpr auto file_str = "\n------> File       : "_sv;

    // We don't want to use the heap here as it might be corrupted
    StackStringBuilder<512> formatted_message;

    formatted_message += assertion_failed;
    formatted_message += expression;
    formatted_message += message;
    formatted_message += function_str;
    formatted_message += function;
    formatted_message += file_str;
    formatted_message += file;
    formatted_message += ':';
    formatted_message += line;

    formatted_message.seal();

    panic(formatted_message.data());
}

[[noreturn]] void panic(const char* reason)
{
    static constexpr auto panic_message = "KERNEL PANIC!"_sv;
    static Color font_color = Color::white();
    static Color screen_color = Color::blue();

    error() << panic_message << "\n" << reason;

    if (InterruptController::is_initialized())
        IPICommunicator::hang_all_cores();

    if (!VideoDevice::is_ready())
        hang();

    auto& surface = VideoDevice::the().surface();
    Rect surface_rect(0, 0, surface.width(), surface.height());
    auto center = surface_rect.center();
    Rect exception_rect(0, 0, center.x(), center.y());
    Point offset = exception_rect.center();
    auto initial_x = offset.x();

    Painter p(&surface);
    p.fill_rect(surface_rect, screen_color);

    Point panic_offset(offset.x() + offset.x() / 2 + (panic_message.size() / 2 * Painter::font_width), offset.y() - 40);

    for (char c : panic_message) {
        p.draw_char(panic_offset, c, font_color, Color::transparent());
        panic_offset.first() += Painter::font_width;
    }

    for (char c : StringView(reason)) {
        if (c == '\n') {
            offset.second() += Painter::font_height;
            offset.first() = initial_x;

            continue;
        }

        p.draw_char(offset, c, font_color, Color::transparent());
        offset.first() += Painter::font_width;
    }

    hang();
}
// clang-format on
}
