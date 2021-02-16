#include "Timer.h"
#include "Core/CPU.h"
#include "Core/Runtime.h"
#include "Interrupts/LAPIC.h"
#include "PIT.h"

namespace kernel {

InterruptSafeSpinLock Timer::s_lock;
Atomic<u64> Timer::s_nanoseconds_since_boot;
Timer::SchedulerEventHandler Timer::s_scheduler_handler;
DynamicArray<Timer::TransparentEventHandler> Timer::s_transparent_handlers;
Atomic<Timer*> Timer::s_primary_timer;
DynamicArray<Timer*> Timer::s_timers;

Timer::Timer(IRQHandler::Type type, u16 irq_index)
    : IRQHandler(type, irq_index)
{
    LOCK_GUARD(s_lock);
    s_timers.append(this);

    if (s_timers.size() == 1)
        make_primary();
}

void Timer::on_tick(const RegisterState& register_state, bool is_bsp)
{
    if (!is_primary())
        return;

    if (is_bsp) {
        LOCK_GUARD(s_lock);

        s_nanoseconds_since_boot += Time::nanoseconds_in_second / current_frequency();

        for (auto handler : s_transparent_handlers)
            handler();
    }

    InterruptController::the().end_of_interrupt(legacy_irq_number());
    s_scheduler_handler(register_state);
}

void Timer::make_primary()
{
    s_primary_timer.store(this);
}

bool Timer::is_primary() const
{
    return s_primary_timer == this;
}

void Timer::discover_and_setup()
{
    // TODO: add HPET and maybe something else?
    new PIT;

    if (InterruptController::the().type() == InterruptController::Type::APIC) {
        auto* lapic_timer = new LAPIC::Timer;
        lapic_timer->calibrate_for_this_processor();
    } else {
        primary().enable();
    }
}

void Timer::register_scheduler_handler(SchedulerEventHandler handler)
{
    LOCK_GUARD(s_lock);
    ASSERT(s_scheduler_handler == nullptr);
    s_scheduler_handler = handler;
}

void Timer::register_handler(TransparentEventHandler handler)
{
    LOCK_GUARD(s_lock);
    s_transparent_handlers.append(handler);
}

void Timer::unregsiter_handler(TransparentEventHandler handler)
{
    LOCK_GUARD(s_lock);

    auto handler_itr = linear_search(s_transparent_handlers.begin(), s_transparent_handlers.end(), handler);

    ASSERT(handler_itr != s_transparent_handlers.end());

    s_transparent_handlers.erase(handler_itr);
}

Timer& Timer::primary()
{
    // this assertion is a bit useless because s_primary_timer can change at any time
    ASSERT(s_primary_timer != nullptr);

    return *s_primary_timer;
}

Timer& Timer::get_specific(Type type)
{
    LOCK_GUARD(s_lock);

    auto timer = linear_search(s_timers.begin(), s_timers.end(), type, [](const Timer* l, Timer::Type r) { return l->type() == r; });

    if (timer != s_timers.end())
        return **timer;

    String error_string;
    error_string << "Failed to find timer of type " << static_cast<size_t>(type);
    runtime::panic(error_string.data());
}
}
