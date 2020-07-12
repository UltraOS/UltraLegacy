#include "Common/Logger.h"
#include "Core/IO.h"

#include "InterruptController.h"
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
    if (ticks_per_second > max_frequency()) {
        error() << "Cannot set the timer to frequency " << ticks_per_second;
        hang();
    }

    m_frequency = ticks_per_second;

    u32 divisor = max_frequency() / ticks_per_second;

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
    increment();

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

    float precise_seconds = static_cast<float>(nanoseconds_since_boot()) / Timer::nanoseconds_in_second;

    if (to_string(precise_seconds, number, 11))
        display_write(number);

    display_write(" seconds");

    // do this here manually since Scheduler::on_tick is very likely to switch the task
    InterruptController::the().end_of_interrupt(irq_index());

    // TODO: make this more pretty
    // e.g the schedulers subscribes on timer events
    // through some kind of a global Timer class or whatever
    Scheduler::on_tick(registers);
}

void PIT::nano_delay(u32 ns)
{
    IO::out8<timer_command>(write_word);

    u32 ticks = max_frequency() * (ns / static_cast<float>(Timer::nanoseconds_in_second));

    if (ticks > 0xFFFF) {
        error() << "PIT: cannot sleep for more than 0xFFFF ticks (got " << format::as_hex << ticks << ")";
        hang();
    }

    IO::out8<timer_data>(ticks & 0xFF);
    IO::out8<timer_data>((ticks & 0xFF00) >> 8);

    static constexpr u16 latch_count_flag = SET_BIT(5);
    static constexpr u16 timer_channel_0 = SET_BIT(1);
    static constexpr u8 timer_done_flag = SET_BIT(7);

    for (;;) {
        IO::out8<timer_command>(write_word | latch_count_flag | timer_channel_0);

        if (IO::in8<timer_data>() & timer_done_flag)
            break;
    }

    // go back to the normal mode
    set_frequency(default_ticks_per_second);
}
}
