#pragma once

#include "Common/Types.h"

#include "Interrupts/Timer.h"
#include "Interrupts/Utilities.h"

#include "Scheduler.h"
#include "Thread.h"

namespace kernel::sleep {

inline void until(u64 time)
{
    Interrupts::ScopedDisabler d;

    if (time <= Timer::nanoseconds_since_boot())
        return;

    Scheduler::the().sleep(time);
}

inline void for_nanoseconds(u64 time)
{
    until(Timer::nanoseconds_since_boot() + time);
}

inline void for_milliseconds(u64 time)
{
    until(Timer::nanoseconds_since_boot() + (time * Time::nanoseconds_in_millisecond));
}

inline void for_seconds(u64 time)
{
    until(Timer::nanoseconds_since_boot() + (time * Time::nanoseconds_in_second));
}
}
