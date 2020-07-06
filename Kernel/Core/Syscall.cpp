#include "Syscall.h"
#include "Common/Logger.h"
#include "Multitasking/Scheduler.h"

namespace kernel {

void Syscall::exit(u8 code)
{
    log() << "Thread " << Thread::current() << " exited with code " << code;

    cli();
    Thread::current()->exit(code);
    Scheduler::yield();
}

void Syscall::debug_log(const char* string)
{
    static auto           cycles = 0;
    static constexpr u8   color  = 0xD;
    static constexpr auto row    = 0x5;

    auto offset = vga_log("Userland process: working... [", row, 0, color);

    static constexpr size_t number_length = 21;
    char                    number[number_length];

    if (to_string(++cycles, number, number_length))
        offset = vga_log(number, row, offset, color);

    offset = vga_log("]", row, offset, color);
    offset = vga_log(" and says: ", row, offset, color);
    vga_log(string, row, offset, color);
}
}
