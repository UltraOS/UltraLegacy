#pragma once

#include "Common/Types.h"

namespace kernel {

class RTC {
public:
    static constexpr u8 seconds_register = 0x00;
    static constexpr u8 minutes_register = 0x02;
    static constexpr u8 hours_register   = 0x04;
    static constexpr u8 weekday_register = 0x06;
    static constexpr u8 day_register     = 0x07;
    static constexpr u8 month_register   = 0x08;
    static constexpr u8 year_register    = 0x09;
    static constexpr u8 century_register = 0x32;

    static constexpr u8 update_in_progress_bit      = 1 << 7;
    static constexpr u8 update_in_progress_register = 0x0A;
    static constexpr u8 time_format_register        = 0x0B;

    static void synchronize_system_clock();

private:
    static bool update_is_in_progress();
};
}
