#pragma once

#include "IRQHandler.h"

namespace kernel {

class PIT : public IRQHandler {
public:
    static constexpr u32 timer_frequency          = 1193180;
    static constexpr u32 default_ticks_per_second = 50;
    static constexpr u8  timer_irq                = 0;

    static constexpr u8 timer_data       = 0x40;
    static constexpr u8 timer_command    = 0x43;
    static constexpr u8 timer_0          = 0x00;
    static constexpr u8 square_wave_mode = 0x06;
    static constexpr u8 write_word       = 0x30;

    PIT();

    void set_frequency(u32 ticks_per_second);

    void on_irq(const RegisterState& registers) override;

private:
    u32 m_frequency { 0 };
};
}
