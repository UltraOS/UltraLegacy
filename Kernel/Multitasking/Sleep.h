#pragma once

#include "Common/Types.h"
#include "Interrupts/Common.h"
#include "Interrupts/Timer.h"
#include "Scheduler.h"
#include "Thread.h"

namespace kernel::sleep {

inline void until(u64 time)
{
    cli();

    if (time <= Timer::nanoseconds_since_boot()) {
        sti();
        return;
    }

    Thread::current()->sleep(time);
    Scheduler::yield();
    sti();
}

inline void for_nanoseconds(u64 time)
{
    until(Timer::nanoseconds_since_boot() + time);
}

inline void for_seconds(u64 time)
{
    until(Timer::nanoseconds_since_boot() + (time * Timer::nanoseconds_in_second));
}
}
