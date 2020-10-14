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

class KernelSymbolTable {
public:
    static constexpr size_t  max_symbols           = 512;
    static constexpr Address physical_symbols_base = 0x45000;

    class Symbol {
    public:
        Symbol() = default;

        Symbol(ptr_t address, char* name) : m_address(address), m_name(name) { }

        ptr_t       address() const { return m_address; }
        const char* name() const { return m_name; }

        friend bool operator<(ptr_t address, const Symbol& symbol) { return address < symbol.address(); }

    private:
        ptr_t m_address { 0 };
        char* m_name { nullptr };
    };

    static bool symbols_available() { return s_symbol_count > 0; }

    static void parse_all()
    {
        static constexpr auto ksyms_begin_magic = "KSYMS"_sv;
        static constexpr auto ksyms_end_magic   = "SMYSK"_sv;

        auto* current_offset = MemoryManager::physical_to_virtual(physical_symbols_base).as_pointer<char>();

        if (StringView(current_offset, ksyms_begin_magic.size()) != ksyms_begin_magic) {
            warning() << "SymbolTable: invalid symbol base magic, kernel symbols won't be available";
            return;
        }
        current_offset += ksyms_begin_magic.size();

        while (StringView(current_offset, ksyms_end_magic.size()) != ksyms_end_magic) {
            if (s_symbol_count == max_symbols) {
                warning() << "SymbolTable: reached the maximum amount of symbols, not all symbols might be available";
                break;
            }

            ptr_t base_of_this_symbol = *reinterpret_cast<ptr_t*>(current_offset);
            current_offset += sizeof(ptr_t);

            s_symbols[s_symbol_count++] = Symbol(base_of_this_symbol, current_offset);

            current_offset += String::length_of(current_offset) + 1;
        }

        if (!symbols_available())
            return;

        s_symbols_begin = s_symbols[0].address();
        s_symbols_end   = s_symbols[s_symbol_count - 1].address();

        log() << "KernelSymbolTable: successfully parsed kernel symbols, " << s_symbol_count << " available.";
    }

    static bool is_address_within_symbol_table(ptr_t address)
    {
        // Add 500 to the last symbol to compensate for the possibility
        // of this address being the last symbol in the table
        return address > s_symbols_begin && address < (s_symbols_end + 500);
    }

    static const Symbol* find_symbol(ptr_t address)
    {
        if (!symbols_available())
            return nullptr;

        if (!is_address_within_symbol_table(address))
            return nullptr;

        for (size_t i = 0; i < s_symbol_count - 1; ++i) {
            if (address < s_symbols[i + 1])
                return &s_symbols[i];
        }

        return &s_symbols[s_symbol_count - 1];
    }

private:
    static ptr_t s_symbols_begin;
    static ptr_t s_symbols_end;

    static size_t s_symbol_count;
    static Symbol s_symbols[max_symbols];
};

ptr_t                     KernelSymbolTable::s_symbols_begin;
ptr_t                     KernelSymbolTable::s_symbols_end;
size_t                    KernelSymbolTable::s_symbol_count;
KernelSymbolTable::Symbol KernelSymbolTable::s_symbols[max_symbols];

void parse_kernel_symbols()
{
    KernelSymbolTable::parse_all();
}

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
ALWAYS_INLINE inline Address base_pointer();
inline Address               base_pointer()
{
    Address base_pointer;

#ifdef ULTRA_32
    asm("mov %%ebp, %0" : "=a"(base_pointer));
#elif defined(ULTRA_64)
    asm("mov %%rbp, %0" : "=a"(base_pointer));
#endif

    return base_pointer;
}

size_t dump_backtrace(ptr_t* into, size_t max_depth, Address base_pointer)
{
    size_t current_depth = 0;

    auto is_address_valid = [](Address adr) -> bool {
        // this doesn't actually guarantee that we won't crash here
        // TODO: figure out a way to make this safer?
        return adr > MemoryManager::kernel_reserved_base;
    };

    // We have to make sure that base pointer is valid before retrieving the instruction pointer
    // because an invalid base indicates the first frame and accessing memory above that is UB
    while (--max_depth && is_address_valid(base_pointer) && is_address_valid(base_pointer.as_pointer<ptr_t>()[0])) {
        into[current_depth++] = base_pointer.as_pointer<ptr_t>()[1];
        base_pointer          = base_pointer.as_pointer<ptr_t>()[0];
    }

    return current_depth;
}

ALWAYS_INLINE inline size_t dump_backtrace(ptr_t* into, size_t max_depth);
inline size_t               dump_backtrace(ptr_t* into, size_t max_depth)
{
    return dump_backtrace(into, max_depth, base_pointer());
}

[[noreturn]] void panic(const char* reason, const RegisterState* registers)
{
    static constexpr auto  panic_message = "KERNEL PANIC!"_sv;
    static constexpr Color font_color    = Color::white();
    static constexpr Color screen_color  = Color::blue();

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

    if (!VideoDevice::is_ready())
        hang();

    auto& surface = VideoDevice::the().surface();
    Rect  surface_rect(0, 0, surface.width(), surface.height());
    auto  center = surface_rect.center();
    Rect  exception_rect(0, 0, center.x(), center.y());
    Point offset    = exception_rect.center();
    auto  initial_x = 10;

    Painter p(&surface);
    p.fill_rect(surface_rect, screen_color);

    Point panic_offset(offset.x() + offset.x() / 2 + (panic_message.size() / 2 * Painter::font_width), offset.y() - 40);

    for (char c: panic_message) {
        p.draw_char(panic_offset, c, font_color, Color::transparent());
        panic_offset.first() += Painter::font_width;
    }

    Logger bt_logger(false);
    offset.first() = initial_x;

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

    if (registers) {
        write_string("Register dump:\n"_sv);
        char reg_as_string[20];

#ifdef ULTRA_32
        to_hex_string(registers->eax, reg_as_string, 20);
        write_string("eax=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->ebx, reg_as_string, 20);
        write_string("ebx=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->ecx, reg_as_string, 20);
        write_string("ecx=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->edx, reg_as_string, 20);
        write_string("edx=");
        write_string(reg_as_string);

        write_string("\n");

        to_hex_string(registers->esi, reg_as_string, 20);
        write_string("esi=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->edi, reg_as_string, 20);
        write_string("edi=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->ebp, reg_as_string, 20);
        write_string("ebp=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->esp, reg_as_string, 20);
        write_string("esp=");
        write_string(reg_as_string);

        write_string("\n");

        to_hex_string(registers->eip, reg_as_string, 20);
        write_string("eip=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->cs, reg_as_string, 20);
        write_string(" cs=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->ss, reg_as_string, 20);
        write_string(" ss=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->gs, reg_as_string, 20);
        write_string(" gs=");
        write_string(reg_as_string);
        write_string(" ");

        write_string("\n");

        to_hex_string(registers->fs, reg_as_string, 20);
        write_string(" fs=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->es, reg_as_string, 20);
        write_string(" es=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->ds, reg_as_string, 20);
        write_string(" ds=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->eflags, reg_as_string, 20);
        write_string("eflags=");
        write_string(reg_as_string);
#elif defined(ULTRA_64)
        to_hex_string(registers->rax, reg_as_string, 20);
        write_string("rax=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->rbx, reg_as_string, 20);
        write_string("rbx=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->rcx, reg_as_string, 20);
        write_string("rcx=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->rdx, reg_as_string, 20);
        write_string("rdx=");
        write_string(reg_as_string);

        write_string("\n");

        to_hex_string(registers->rsi, reg_as_string, 20);
        write_string("rsi=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->rdi, reg_as_string, 20);
        write_string("rdi=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->rbp, reg_as_string, 20);
        write_string("rbp=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->r8, reg_as_string, 20);
        write_string(" r8=");
        write_string(reg_as_string);

        write_string("\n");

        to_hex_string(registers->r9, reg_as_string, 20);
        write_string(" r9=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->r10, reg_as_string, 20);
        write_string("r10=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->r11, reg_as_string, 20);
        write_string("r11=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->r12, reg_as_string, 20);
        write_string("r12=");
        write_string(reg_as_string);

        write_string("\n");

        to_hex_string(registers->r13, reg_as_string, 20);
        write_string("r13=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->r14, reg_as_string, 20);
        write_string("r14=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->r15, reg_as_string, 20);
        write_string("r15=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->rsp, reg_as_string, 20);
        write_string("rsp=");
        write_string(reg_as_string);

        write_string("\n");

        to_hex_string(registers->rip, reg_as_string, 20);
        write_string("rip=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->cs, reg_as_string, 20);
        write_string(" cs=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->ss, reg_as_string, 20);
        write_string(" ss=");
        write_string(reg_as_string);
        write_string(" ");

        to_hex_string(registers->rflags, reg_as_string, 20);
        write_string("rflags=");
        write_string(reg_as_string);
#endif

        write_string("\n\n");
    }

    write_string("Backtrace:\n"_sv);

    static constexpr size_t backtrace_max_depth = 8;
    ptr_t                   backtrace[backtrace_max_depth] {};
    size_t                  depth;

    if (registers) {
        backtrace[0] = instruction_pointer(*registers);
        depth        = dump_backtrace(backtrace + 1, backtrace_max_depth - 1, base_pointer(*registers)) + 1;
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
