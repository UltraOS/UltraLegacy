#include "Timer.h"
#include "Core/Runtime.h"
#include "Core/CPU.h"
#include "PIT.h"


namespace kernel {

Timer* Timer::s_timer;

void Timer::discover_and_setup()
{
    // TODO: add HPET and maybe something else?
    s_timer = new PIT;

    // If the CPU doesn't support SMP then we just use
    // the BSP-only timer for interrupts and scheduling, which is
    // either PIT or HPET, otherwise we keep the IRQs masked and just
    // use the timer either for counter or for precise sleeping
    if (!CPU::supports_smp())
        s_timer->enable();
}

Timer& Timer::the()
{
    ASSERT(s_timer != nullptr);

    return *s_timer;
}
}
