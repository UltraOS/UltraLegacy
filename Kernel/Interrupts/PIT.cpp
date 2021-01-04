#include "Common/Logger.h"
#include "Core/IO.h"
#include "Memory/MemoryManager.h"

#include "PIT.h"

namespace kernel {

PIT::PIT()
    : Timer(irq_number)
{
    set_frequency(default_ticks_per_second);
}

void PIT::set_frequency(u32 ticks_per_second)
{
    if (ticks_per_second > max_frequency()) {
        String error_string;
        error_string << "PIT: Cannot set the timer to frequency " << ticks_per_second;
        runtime::panic(error_string.data());
    }

    m_frequency = ticks_per_second;

    u32 divisor = max_frequency() / ticks_per_second;

    if (divisor > 0xFFFF) {
        String error_string;
        error_string << "Timer: divisor is too big (" << divisor << ")";
        runtime::panic(error_string.data());
    }

    IO::out8<timer_command>(write_word | square_wave_mode | timer_0);

    IO::out8<timer_data>(divisor & 0x000000FF);
    IO::out8<timer_data>((divisor & 0x0000FF00) >> 8);
}

void PIT::nano_delay(u32 ns)
{
    // TODO: this delay is actually longer than 'ns' because of the sleep preparation
    //       find out the amount of ticks we want to subtract to make it more accurate

    IO::out8<timer_command>(write_word);

    // TODO: Remove this, we don't want to use floats in the kernel
    u32 ticks = max_frequency() * (ns / static_cast<float>(Time::nanoseconds_in_second));

    if (ticks > 0xFFFF) {
        String error_string;
        error_string << "PIT: cannot sleep for more than 0xFFFF ticks (got " << format::as_hex << ticks << ")";
        runtime::panic(error_string.data());
    }

    IO::out8<timer_data>(ticks & 0xFF);
    IO::out8<timer_data>((ticks & 0xFF00) >> 8);

    static constexpr u8 latch_count_flag = SET_BIT(5);
    static constexpr u8 timer_channel_0 = SET_BIT(1);
    static constexpr u8 timer_done_flag = SET_BIT(7);
    static constexpr u8 read_back_command = SET_BIT(7) | SET_BIT(6);

    for (;;) {
        IO::out8<timer_command>(read_back_command | latch_count_flag | timer_channel_0);

        if (IO::in8<timer_data>() & timer_done_flag)
            break;
    }

    // go back to the normal mode
    set_frequency(default_ticks_per_second);
}
}
