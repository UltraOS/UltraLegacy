#pragma once

#include "IRQHandler.h"
#include "Timer.h"

namespace kernel {

class PIT final : public Timer {
public:
    PIT();

    static constexpr u32 frequency = 1193180;
    // TODO: replace with numeric_limits<u16>::max();
    static constexpr u16 max_divisor = 0xFFFF;
    static constexpr u8 irq_number = 0;

    // ---- I/O Ports ----
    static constexpr u8 timer_data = 0x40;
    static constexpr u8 timer_command = 0x43;
    static constexpr u8 timer_0 = 0x00;

    static constexpr u8 square_wave_mode = 0x06;
    static constexpr u8 write_word = 0x30;

    void set_frequency(u32 ticks_per_second) override;

    u32 max_frequency() const override { return frequency; }

    u32 current_frequency() const override { return m_frequency; }

    bool is_per_cpu() const override { return false; }
    StringView model() const override { return "PIT (8254)"_sv; };
    Type type() const override { return Type::PIT; }

    void enable() override { enable_irq(); }
    void disable() override { disable_irq(); }

    size_t minimum_delay_ns() const override { return 10 * Time::nanoseconds_in_microsecond; }
    size_t maximum_delay_ns() const override { return max_divisor / ticks_in_10_microseconds * 10 * Time::nanoseconds_in_microsecond; }
    void nano_delay(u32 ns) override;

    ~PIT() override { disable_irq(); }

private:
    static constexpr u32 ticks_in_10_microseconds = 12;

    u32 m_frequency { 0 };
};
}
