#include "Common/Logger.h"
#include "Core/IO.h"

#include "InterruptController.h"
#include "Memory/MemoryManager.h"
#include "Multitasking/Scheduler.h"

#include "PIT.h"

namespace kernel {

PIT::PIT() : IRQHandler(irq_number)
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

u64 PIT::nanoseconds_since_boot()
{
    return m_nanoseconds_since_boot;
}

void PIT::handle_irq(const RegisterState& registers)
{
    // TODO: it's really annoying to have to increment a bunch of clocks in every timer.
    //       find a way to make it better.
    auto time_elapsed = Time::nanoseconds_in_second / current_frequency();

    m_nanoseconds_since_boot += time_elapsed;
    Time::increment_by(time_elapsed);

    // do this here manually since Scheduler::on_tick is very likely to switch the task
    InterruptController::the().end_of_interrupt(irq_index());

    // TODO: make this more pretty
    // e.g the schedulers subscribes on timer events
    // through some kind of a global Timer class or whatever
    Scheduler::on_tick(registers);
}

void PIT::nano_delay(u32 ns)
{
    // TODO: this delay is actually longer than 'ns' because of the sleep preparation
    //       find out the amount of ticks we want to subtract to make it more accurate

    IO::out8<timer_command>(write_word);

    // TODO: Remove this, we don't want to use floats in the kernel
    u32 ticks = max_frequency() * (ns / static_cast<float>(Time::nanoseconds_in_second));

    if (ticks > 0xFFFF) {
        error() << "PIT: cannot sleep for more than 0xFFFF ticks (got " << format::as_hex << ticks << ")";
        hang();
    }

    IO::out8<timer_data>(ticks & 0xFF);
    IO::out8<timer_data>((ticks & 0xFF00) >> 8);

    static constexpr u8 latch_count_flag  = SET_BIT(5);
    static constexpr u8 timer_channel_0   = SET_BIT(1);
    static constexpr u8 timer_done_flag   = SET_BIT(7);
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
