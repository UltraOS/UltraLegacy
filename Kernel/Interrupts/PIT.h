#pragma once

#include "IRQHandler.h"
#include "Timer.h"

namespace kernel {

class PIT final : public Timer, public IRQHandler {
public:
    static constexpr u32 frequency = 1193180;
    // TODO: replace with numeric_limits<u16>::max();
    static constexpr u16 max_divisor = 0xFFFF;
    static constexpr u8  timer_irq   = 0;

    // ---- I/O Ports ----
    static constexpr u8 timer_data       = 0x40;
    static constexpr u8 timer_command    = 0x43;
    static constexpr u8 timer_0          = 0x00;
    static constexpr u8 square_wave_mode = 0x06;
    static constexpr u8 write_word       = 0x30;

    PIT();

    void set_frequency(u32 ticks_per_second) override;

    u32 max_frequency() const override { return frequency; }

    u32 current_frequency() const override { return m_frequency; }

    StringView model() const override { return "PIT (8254)"_sv; };

    void enable() override { enable_irq(); }
    void disable() override { disable_irq(); }

    // don't do anything since we finalize manually
    void finialize_irq() override { }

    void on_irq(const RegisterState& registers) override;

    ~PIT() { disable_irq(); }

private:
    u32 m_frequency { 0 };
};
}
