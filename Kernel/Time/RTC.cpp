#include "Core/CMOS.h"

#include "Common/Conversions.h"
#include "Common/Logger.h"

#include "Interrupts/Utilities.h"

#include "RTC.h"
#include "Time.h"

namespace kernel {

bool RTC::update_is_in_progress()
{
    return CMOS::read<update_in_progress_register>() & update_in_progress_bit;
}

void RTC::synchronize_system_clock()
{
    Interrupts::ScopedDisabler d;

    Time::HumanReadable original_time;
    Time::HumanReadable additional_time;

    auto time_format = CMOS::read<time_format_register>();

    static constexpr u8 hour_24_bit = 1 << 1;
    static constexpr u8 bcd_bit     = 1 << 2;

    bool is_12_hours = !(time_format & hour_24_bit);
    bool is_bcd      = !(time_format & bcd_bit);

    while (update_is_in_progress())
        ;

    original_time.second = CMOS::read<seconds_register>();
    original_time.minute = CMOS::read<minutes_register>();
    original_time.hour   = CMOS::read<hours_register>();
    original_time.day    = CMOS::read<day_register>();
    original_time.month  = static_cast<Time::Month>(CMOS::read<month_register>());
    original_time.year   = CMOS::read<year_register>();
    // TODO: read century register if present

    // clang-format off
    do {
        additional_time = original_time;

        while (update_is_in_progress())
            ;

        original_time.second = CMOS::read<seconds_register>();
        original_time.minute = CMOS::read<minutes_register>();
        original_time.hour   = CMOS::read<hours_register>();
        original_time.day    = CMOS::read<day_register>();
        original_time.month  = static_cast<Time::Month>(CMOS::read<month_register>());
        original_time.year   = CMOS::read<year_register>();
        // TODO: read century register if present
    } while (original_time.second != additional_time.second ||
             original_time.minute != additional_time.minute ||
             original_time.hour   != additional_time.hour   ||
             original_time.day    != additional_time.day    ||
             original_time.year   != additional_time.year);
    // clang-format on

    if (is_bcd) {
        original_time.second = bcd_to_binary(original_time.second);
        original_time.minute = bcd_to_binary(original_time.minute);
        original_time.hour   = bcd_to_binary(original_time.hour);
        original_time.day    = bcd_to_binary(original_time.day);
        original_time.month  = static_cast<Time::Month>(bcd_to_binary(static_cast<u8>(original_time.month)));
        original_time.year   = bcd_to_binary(original_time.year);
    }

    original_time.year += 2000;

    static constexpr u8 pm_bit = 1 << 7;

    if (is_12_hours && (original_time.hour & pm_bit))
        original_time.hour = ((original_time.hour & ~pm_bit) + 12) % 24;

    Time::set(Time::to_unix(original_time));
}
}
