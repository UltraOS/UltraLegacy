#pragma once

#include "IRQHandler.h"
#include "Timer.h"

namespace kernel {

class PIT final : public Timer, public IRQHandler {
    MAKE_SINGLETON_INHERITABLE(Timer, PIT);

public:
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

    StringView model() const override { return "PIT (8254)"_sv; };
    bool has_internal_counter() override { return false; }

    u64 nanoseconds_since_boot() override;
    void increment_time_since_boot(u64 nanoseconds) override { m_nanoseconds_since_boot += nanoseconds; }

    void enable() override { enable_irq(); }
    void disable() override { disable_irq(); }

    void nano_delay(u32 ns) override;

    // don't do anything since we finalize manually
    void finalize_irq() override { }

    void handle_irq(const RegisterState& registers) override;

    ~PIT() { disable_irq(); }

private:
    u64 m_nanoseconds_since_boot { 0 };
    u32 m_frequency { 0 };
};
}
