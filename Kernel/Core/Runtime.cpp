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

constexpr size_t magic_length = 13;
extern const char magic_string[magic_length];

namespace kernel::runtime {
// clang-format off
void ensure_loaded_correctly()
{
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
        log() << "Runtime: Magic test passed!";
    } else {
        error() << "Runtime: Magic test failed!";
        hang();
    }
}

void init_global_objects()
{
    for (global_constructor_t* ctor = &global_constructors_begin; ctor < &global_constructors_end; ++ctor)
        (*ctor)();
}
// clang-format on
void KernelSymbolTable::parse_all()
{
    static constexpr auto ksyms_begin_magic = "KSYMS"_sv;
    static constexpr auto ksyms_end_magic = "SMYSK"_sv;

    static constexpr size_t symbol_failsafe_count = 10000;

    auto* symbols_pointer = MemoryManager::physical_to_virtual(physical_symbols_base).as_pointer<char>();

    if (StringView(symbols_pointer, ksyms_begin_magic.size()) != ksyms_begin_magic) {
        warning() << "SymbolTable: invalid symbol base magic, kernel symbols won't be available";
        return;
    }

    symbols_pointer += ksyms_begin_magic.size();

    s_symbols = Address(symbols_pointer).as_pointer<Symbol>();

    while (StringView(symbols_pointer, ksyms_end_magic.size()) != ksyms_end_magic) {
        s_symbol_count++;
        symbols_pointer += sizeof(Symbol);

        if (s_symbol_count > symbol_failsafe_count) {
            warning() << "SymbolTable: Cannot find end of table, ignoring all symbols";
            s_symbol_count = 0;
            s_symbols = nullptr;
            return;
        }
    }

    if (!symbols_available())
        return;

    s_symbols_begin = s_symbols[0].address();
    s_symbols_end = s_symbols[s_symbol_count - 1].address();

    log() << "KernelSymbolTable: successfully parsed kernel symbols, " << s_symbol_count << " available.";
}

bool KernelSymbolTable::is_address_within_symbol_table(ptr_t address)
{
    // Add 500 to the last symbol to compensate for the possibility
    // of this address being the last symbol in the table
    return address > s_symbols_begin && address < (s_symbols_end + 500);
}

const KernelSymbolTable::Symbol* KernelSymbolTable::find_symbol(ptr_t address)
{
    if (!symbols_available())
        return nullptr;

    if (!is_address_within_symbol_table(address))
        return nullptr;

    auto* symbol = lower_bound(begin(), end(), address);

    if (symbol == end() || symbol->address() != address)
        --symbol;

    return symbol;
}

Address KernelSymbolTable::s_symbols_begin;
Address KernelSymbolTable::s_symbols_end;
size_t KernelSymbolTable::s_symbol_count;
KernelSymbolTable::Symbol* KernelSymbolTable::s_symbols;

void on_assertion_failed(const char* message, const char* file, const char* function, u32 line)
{
    static constexpr auto assertion_failed = "Assertion failed!"_sv;
    static constexpr auto expression = "\n------> Expression : "_sv;
    static constexpr auto function_str = "\n------> Function   : "_sv;
    static constexpr auto file_str = "\n------> File       : "_sv;

    // We don't want to use the heap here as it might be corrupted
    StackStringBuilder<512> formatted_message;
    formatted_message << assertion_failed << expression << message << function_str << function << file_str << file
                      << ':' << line;

    panic(formatted_message.data());
}

size_t dump_backtrace(ptr_t* into, size_t max_depth, Address base_pointer)
{
    size_t current_depth = 0;

    auto is_address_valid = [](Address adr) -> bool {
        // this doesn't actually guarantee that we won't crash here
        // TODO: figure out a way to make this safer?
        return adr >= MemoryManager::kernel_address_space_base;
    };

    // We have to make sure that base pointer is valid before retrieving the instruction pointer
    // because an invalid base indicates the first frame and accessing memory above that is UB
    while (--max_depth && is_address_valid(base_pointer) && is_address_valid(base_pointer.as_pointer<ptr_t>()[0])) {
        into[current_depth++] = base_pointer.as_pointer<ptr_t>()[1];
        base_pointer = base_pointer.as_pointer<ptr_t>()[0];
    }

    return current_depth;
}

[[noreturn]] void panic(const char* reason, const RegisterState* registers)
{
    static constexpr auto panic_message = "KERNEL PANIC!"_sv;
    static constexpr Color font_color = Color::white();
    static constexpr Color screen_color = Color::blue();

    Interrupts::disable();

    // TODO: make this bool atomic
    static bool in_panic = false;
    if (in_panic) {
        error() << "Panicked while inside panic: " << reason;
        hang();
    }
    in_panic = true;

    error() << panic_message << "\n";

    if (InterruptController::is_initialized())
        IPICommunicator::hang_all_cores();

    Rect surface_rect;
    Point center;
    Rect exception_rect;
    Point offset;
    bool can_draw = VideoDevice::is_ready();

    if (can_draw) {
        auto& surface = VideoDevice::the().surface();
        surface_rect = { 0, 0, surface.width(), surface.height() };
        center = surface_rect.center();
        exception_rect = { 0, 0, center.x(), center.y() };
        offset = exception_rect.center();
    }

    auto initial_x = 10;

    alignas(Painter) u8 optional_painter[sizeof(Painter)];

    if (can_draw)
        new (optional_painter) Painter(&VideoDevice::the().surface());

#define TRY_PAINT(what) \
    if (can_draw)       \
    reinterpret_cast<Painter*>(optional_painter)->what

    TRY_PAINT(fill_rect(surface_rect, screen_color));

    Point panic_offset(offset.x() + offset.x() / 2 + (panic_message.size() / 2 * Painter::font_width), offset.y() - 40);

    for (char c : panic_message) {
        TRY_PAINT(draw_char(panic_offset, c, font_color, Color::transparent()));
        panic_offset.first() += Painter::font_width;
    }

    Logger bt_logger(false);
    offset.first() = initial_x;

    auto write_string = [&](StringView string) {
        bt_logger << string;
        for (char c : StringView(string)) {
            if (c == '\n') {
                offset.second() += Painter::font_height;
                offset.first() = initial_x;

                continue;
            }

            TRY_PAINT(draw_char(offset, c, font_color, Color::transparent()));
            offset.first() += Painter::font_width;
        }
    };

    write_string(reason);

    offset.second() += Painter::font_height * 2;
    offset.first() = initial_x;

    bt_logger << "\n\n"_sv;

    if (registers) {
        static constexpr size_t max_number_width = 20;

        char reg_as_string[max_number_width];

        write_string("Register dump:\n"_sv);

        auto trace_reg = [&](size_t reg, StringView repr) {
            to_hex_string(reg, reg_as_string, max_number_width);

            // padding
            if (repr.size() == 2)
                write_string(" ");

            write_string(repr);
            write_string("=");
            write_string(reg_as_string);
            write_string(" ");
        };

// clang-format off
#define TRACE_REG(reg) trace_reg(registers->reg, #reg ## _sv)
#define NL write_string("\n");

#ifdef ULTRA_32
        TRACE_REG(eax); TRACE_REG(ebx); TRACE_REG(ecx); TRACE_REG(edx);    NL
        TRACE_REG(esi); TRACE_REG(edi); TRACE_REG(ebp); TRACE_REG(esp);    NL
        TRACE_REG(eip); TRACE_REG(cs);  TRACE_REG(ss);  TRACE_REG(gs);     NL
        TRACE_REG(fs);  TRACE_REG(es);  TRACE_REG(ds);  TRACE_REG(eflags); NL
#elif defined(ULTRA_64)
        TRACE_REG(rax); TRACE_REG(rbx); TRACE_REG(rcx); TRACE_REG(rdx);    NL
        TRACE_REG(rsi); TRACE_REG(rdi); TRACE_REG(rbp); TRACE_REG(rsp);    NL
        TRACE_REG(r8);  TRACE_REG(r9);  TRACE_REG(r10); TRACE_REG(r11);    NL
        TRACE_REG(r12); TRACE_REG(r13); TRACE_REG(r14); TRACE_REG(r15);    NL
        TRACE_REG(rip); TRACE_REG(cs);  TRACE_REG(ss);  TRACE_REG(rflags); NL
#endif
            // clang-format on

            write_string("\n");
    }

    write_string("Backtrace:\n"_sv);

    static constexpr size_t backtrace_max_depth = 8;
    ptr_t backtrace[backtrace_max_depth] {};
    size_t depth;

    if (registers) {
        backtrace[0] = registers->instruction_pointer();
        depth = dump_backtrace(backtrace + 1, backtrace_max_depth - 1, registers->base_pointer()) + 1;
    } else {
        depth = dump_backtrace(backtrace, backtrace_max_depth, base_pointer());
    }

    if (!depth) {
        write_string("No backtrace available (possible stack corruption)"_sv);
        hang();
    }

    static constexpr auto frame_message = "Frame "_sv;

    char number[20];

    for (size_t current_depth = 0; current_depth < depth; ++current_depth) {
        write_string(frame_message);

        to_string(current_depth, number, 20);
        write_string(number);
        write_string(": "_sv);
        to_hex_string(backtrace[current_depth], number, 20);
        write_string(number);
        write_string(" in "_sv);
        auto* symbol = KernelSymbolTable::find_symbol(backtrace[current_depth]);
        if (symbol)
            write_string(symbol->name());
        else
            write_string("??");
        write_string("\n"_sv);
    }

    hang();
}
}
