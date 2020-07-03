#include "Timer.h"
#include "Core/Runtime.h"
#include "PIT.h"

namespace kernel {

u64    Timer::s_nanoseconds_since_boot;
Timer* Timer::s_timer;

void Timer::discover_and_setup()
{
    // TODO: add HPET and maybe something else?
    s_timer = new PIT;
    s_timer->set_frequency(default_ticks_per_second);
    s_timer->enable();
}

Timer& Timer::the()
{
    ASSERT(s_timer != nullptr);

    return *s_timer;
}

void Timer::increment()
{
    s_nanoseconds_since_boot += nanoseconds_in_second / current_frequency();
}

u64 Timer::nanoseconds_since_boot()
{
    return s_nanoseconds_since_boot;
}
}
