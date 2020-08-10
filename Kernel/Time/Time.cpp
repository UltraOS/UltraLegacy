#include "Time.h"
#include "Common/Logger.h"
namespace kernel {

Time::time_t Time::s_ns;
Time::time_t Time::s_now;

bool Time::is_leap_year(u16 year)
{
    if (year % 4)
        return false;
    else if (year % 100)
        return true;
    else if (year % 400)
        return false;
    else
        return true;
}

u32 Time::days_in_years_since_epoch_before(u16 current_year)
{
    --current_year;

    u32 days_total = 0;

    for (; current_year >= epoch_year; --current_year) {
        days_total += is_leap_year(current_year) ? 366 : 365;
    }

    return days_total;
}

u32 Time::days_in_months_since_start_of(u16 year, Month current)
{
    u32 days_total = 0;

    --current;

    switch (current) {
    case Month::December: ASSERT(false && "Current month cannot be > 12");
    case Month::November: days_total += 30; [[fallthrough]];
    case Month::October: days_total += 31; [[fallthrough]];
    case Month::September: days_total += 30; [[fallthrough]];
    case Month::August: days_total += 31; [[fallthrough]];
    case Month::July: days_total += 31; [[fallthrough]];
    case Month::June: days_total += 30; [[fallthrough]];
    case Month::May: days_total += 31; [[fallthrough]];
    case Month::April: days_total += 30; [[fallthrough]];
    case Month::March: days_total += 31; [[fallthrough]];
    case Month::February: days_total += is_leap_year(year) ? 29 : 28; [[fallthrough]];
    case Month::January: days_total += 31;
    }

    return days_total;
}

u8 Time::days_in_month(Month month, u16 of_year)
{
    switch (month) {
    case Month::December: return 31;
    case Month::November: return 30;
    case Month::October: return 31;
    case Month::September: return 30;
    case Month::August: return 31;
    case Month::July: return 31;
    case Month::June: return 30;
    case Month::May: return 31;
    case Month::April: return 30;
    case Month::March: return 31;
    case Month::February: return is_leap_year(of_year) ? 29 : 28;
    case Month::January: return 31;
    default: return 0;
    }
}

Time::time_t Time::to_unix(const HumanReadable& time)
{
    return days_in_years_since_epoch_before(time.year) * seconds_in_day
           + days_in_months_since_start_of(time.year, time.month) * seconds_in_day + (time.day - 1) * seconds_in_day
           + time.hour * seconds_in_hour + time.minute * seconds_in_minute + time.second;
}

Time::HumanReadable Time::from_unix(time_t time)
{
    HumanReadable hr_time {};
    hr_time.year  = epoch_year;
    hr_time.day   = 1;
    hr_time.month = Month::January;

    static constexpr auto seconds_in_year      = seconds_in_day * 365;
    static constexpr auto seconds_in_leap_year = seconds_in_day * 366;

    while (time >= seconds_in_year) {
        bool is_leap = is_leap_year(hr_time.year++);

        if (is_leap && (time < seconds_in_leap_year)) {
            hr_time.year--;
            break;
        }

        time -= is_leap ? seconds_in_leap_year : seconds_in_year;
    }

    for (;;) {
        auto seconds_in_this_month = days_in_month(hr_time.month++, hr_time.year) * seconds_in_day;

        if (time < seconds_in_this_month) {
            --hr_time.month;
            break;
        }

        time -= seconds_in_this_month;
    }

    hr_time.day = time / seconds_in_day + 1;
    if (hr_time.day > 1)
        time -= (hr_time.day - 1) * seconds_in_day;

    hr_time.hour = time / seconds_in_hour;
    if (hr_time.hour)
        time -= hr_time.hour * seconds_in_hour;

    hr_time.minute = time / seconds_in_minute;
    if (hr_time.minute)
        time -= hr_time.minute * seconds_in_minute;

    hr_time.second = time;

    return hr_time;
}

void Time::check_if_second_passed()
{
    auto seconds_passed = s_ns / nanoseconds_in_second;

    if (seconds_passed) {
        s_ns -= seconds_passed * nanoseconds_in_second;
        s_now += seconds_passed;

        vga_write_time();
    }
}

void Time::vga_write_time()
{
    auto time  = now_readable();
    bool is_pm = false;

    char digit[8];

    if (time.hour >= 12) {
        is_pm = true;

        if (time.hour != 12)
            time.hour -= 12;
    }

    to_string(time.hour, digit, 8);
    if (time.hour < 10) {
        digit[1] = digit[0];
        digit[0] = '0';
        digit[2] = '\0';
    }

    auto offset = vga_log(digit, 0, 0, 0x9);
    offset      = vga_log(":", 0, offset, 0x9);

    to_string(time.minute, digit, 8);
    if (time.minute < 10) {
        digit[1] = digit[0];
        digit[0] = '0';
        digit[2] = '\0';
    }

    offset = vga_log(digit, 0, offset, 0x9);
    offset = vga_log(":", 0, offset, 0x9);

    to_string(time.second, digit, 8);
    if (time.second < 10) {
        digit[1] = digit[0];
        digit[0] = '0';
        digit[2] = '\0';
    }

    offset = vga_log(digit, 0, offset, 0x9);

    offset = vga_log(is_pm ? " PM " : " AM ", 0, offset, 0x9);

    to_string(time.day, digit, 8);
    if (time.day < 10) {
        digit[1] = digit[0];
        digit[0] = '0';
        digit[2] = '\0';
    }

    offset = vga_log(digit, 0, offset, 0x9);
    offset = vga_log(".", 0, offset, 0x9);

    to_string(static_cast<u8>(time.month), digit, 8);
    if (static_cast<u8>(time.month) < 10) {
        digit[1] = digit[0];
        digit[0] = '0';
        digit[2] = '\0';
    }

    offset = vga_log(digit, 0, offset, 0x9);
    offset = vga_log(".", 0, offset, 0x9);

    to_string(time.year, digit, 8);
    vga_log(digit, 0, offset, 0x9);
}
}
