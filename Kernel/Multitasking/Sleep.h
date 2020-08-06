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

    if (time <= Timer::the().nanoseconds_since_boot())
        return;

    Thread::current()->sleep(time);
    Scheduler::yield();
}

inline void for_nanoseconds(u64 time)
{
    until(Timer::the().nanoseconds_since_boot() + time);
}

inline void for_seconds(u64 time)
{
    until(Timer::the().nanoseconds_since_boot() + (time * Timer::nanoseconds_in_second));
}
}
