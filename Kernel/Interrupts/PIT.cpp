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

    if (divisor > max_divisor) {
        String error_string;
        error_string << "Timer: divisor is too big (" << divisor << ")";
        runtime::panic(error_string.data());
    }

    IO::out8<timer_command>(write_word | square_wave_mode | timer_0);

    IO::out8<timer_data>(divisor & 0x000000FFu);
    IO::out8<timer_data>((divisor & 0x0000FF00u) >> 8u);
}

void PIT::nano_delay(u32 ns)
{
    if (ns < minimum_delay_ns() || ns > maximum_delay_ns()) {
        String error_string;
        error_string << "PIT: invalid sleep duration " << ns
                     << " ns, expected a value in range "
                     << minimum_delay_ns() << " - " << maximum_delay_ns();
        runtime::panic(error_string.data());
    }
    
    u32 ticks_to_sleep = (ns / (Time::nanoseconds_in_microsecond * 10)) * ticks_in_10_microseconds;

    IO::out8<timer_command>(write_word);
    IO::out8<timer_data>(ticks_to_sleep & 0xFFu);
    IO::out8<timer_data>((ticks_to_sleep & 0xFF00u) >> 8u);

    static constexpr u8 latch_count_flag = SET_BIT(5u);
    static constexpr u8 timer_channel_0 = SET_BIT(1u);
    static constexpr u8 timer_done_flag = SET_BIT(7u);
    static constexpr u8 read_back_command = SET_BIT(7u) | SET_BIT(6u);

    for (;;) {
        IO::out8<timer_command>(read_back_command | latch_count_flag | timer_channel_0);

        if (IO::in8<timer_data>() & timer_done_flag)
            break;
    }

    // go back to the normal mode
    set_frequency(default_ticks_per_second);
}
}
