#include "Common/Logger.h"
#include "Core/IO.h"

#include "Memory/MemoryManager.h"
#include "Multitasking/Scheduler.h"

#include "PIT.h"

namespace kernel {

PIT::PIT() : IRQHandler(timer_irq)
{
    set_frequency(default_ticks_per_second);
}

void PIT::set_frequency(u32 ticks_per_second)
{
    if (ticks_per_second > timer_frequency) {
        error() << "Cannot set the timer to frequency " << ticks_per_second;
        hang();
    }

    m_frequency = ticks_per_second;

    u32 divisor = timer_frequency / ticks_per_second;

    if (divisor > 0xFFFF) {
        error() << "Timer: divisor is too big (" << divisor << ")";
        hang();
    }
    IO::out8<timer_command>(write_word | square_wave_mode | timer_0);

    IO::out8<timer_data>(divisor & 0x000000FF);
    IO::out8<timer_data>((divisor & 0x0000FF00) >> 8);
}

void PIT::on_irq(const RegisterState& registers)
{
    static u32 tick = 0;

    ++tick;

    auto display_write = [](const char* string, bool carriage_return = false) {
        static constexpr ptr_t vga_address        = 0xB8000;
        static constexpr ptr_t linear_vga_address = MemoryManager::physical_address_as_kernel(vga_address);
        static constexpr u8    light_blue         = 0x9;

        static u16* memory = reinterpret_cast<u16*>(linear_vga_address);

        if (carriage_return)
            memory = reinterpret_cast<u16*>(linear_vga_address);

        while (*string) {
            u16 colored_char = *(string++);
            colored_char |= light_blue << 8;

            *(memory++) = colored_char;
        }
    };

    display_write("Uptime: ", true);

    char number[11];

    float precise_seconds = static_cast<float>(tick) / m_frequency;

    if (to_string(precise_seconds, number, 11))
        display_write(number);

    display_write(" seconds");

    // TODO: make this more pretty
    // e.g the schedulers subscribes on timer events
    // through some kind of a global Timer class or whatever
    Scheduler::on_tick(registers);
}
}
