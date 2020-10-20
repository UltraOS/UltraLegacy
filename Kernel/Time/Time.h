#pragma once

#include "Common/String.h"
#include "Common/Types.h"

namespace kernel {
class Time {
    MAKE_STATIC(Time);

public:
    static constexpr size_t nanoseconds_in_microsecond = 1000;
    static constexpr size_t nanoseconds_in_millisecond = 1000000;
    static constexpr size_t nanoseconds_in_second      = 1000000000;
    static constexpr size_t milliseconds_in_second     = 1000;
    static constexpr size_t seconds_in_day             = 86400;
    static constexpr size_t seconds_in_hour            = 3600;
    static constexpr size_t seconds_in_minute          = 60;

    static constexpr u16 epoch_year = 1970;

    enum class Month : u8 {
        January = 1,
        February,
        March,
        April,
        May,
        June,
        July,
        August,
        September,
        October,
        November,
        December
    };

    friend Month& operator--(Month& m)
    {
        auto prior = static_cast<u8>(m);
        --prior;

        m = static_cast<Month>(prior);
        return m;
    }

    friend Month& operator++(Month& m)
    {
        auto next = static_cast<u8>(m);
        ++next;

        m = static_cast<Month>(next);
        return m;
    }

    friend Month operator++(Month& m, int)
    {
        auto next = static_cast<u8>(m);
        ++next;

        auto current = m;
        m            = static_cast<Month>(next);

        return current;
    }

    struct HumanReadable {
        u16   year;
        Month month;
        u8    day;
        u8    hour;
        u8    minute;
        u8    second;
    };

    constexpr StringView month_to_string(Month month)
    {
        switch (month) {
        case Month::January: return "January"_sv;
        case Month::February: return "February"_sv;
        case Month::March: return "March"_sv;
        case Month::April: return "April"_sv;
        case Month::May: return "May"_sv;
        case Month::June: return "June"_sv;
        case Month::July: return "July"_sv;
        case Month::August: return "August"_sv;
        case Month::September: return "September"_sv;
        case Month::October: return "October"_sv;
        case Month::November: return "November"_sv;
        case Month::December: return "December"_sv;
        default: return "Unknown Month"_sv;
        }
    }

    using time_t = u64;

    static bool is_leap_year(u16 year);
    static u32  days_in_years_since_epoch_before(u16 current_year);
    static u32  days_in_months_since_start_of(u16 year, Month current);
    static u8   days_in_month(Month, u16 of_year);

    static void set(u64 now) { s_now = now; }

    static void increment_by(u64 ns)
    {
        s_ns += ns;
        check_if_second_passed();
    }

    static time_t        to_unix(const HumanReadable&);
    static HumanReadable from_unix(time_t);

    static time_t        now() { return s_now; }
    static HumanReadable now_readable() { return from_unix(s_now); }

private:
    static void check_if_second_passed();

private:
    static time_t s_ns;
    static time_t s_now;
};
}
