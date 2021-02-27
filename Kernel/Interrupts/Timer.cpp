#include "Timer.h"
#include "Core/CPU.h"
#include "Core/Runtime.h"
#include "Interrupts/APIC.h"
#include "Interrupts/LAPIC.h"
#include "PIT.h"

namespace kernel {

InterruptSafeSpinLock Timer::s_lock;
Atomic<u64> Timer::s_nanoseconds_since_boot;
Timer::SchedulerEventHandler Timer::s_scheduler_handler;
DynamicArray<Timer::TransparentEventHandler> Timer::s_transparent_handlers;

Timer::Timer(IRQHandler::Type type, u16 irq_index)
    : IRQHandler(type, irq_index)
    , Device(Category::TIMER)
{
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

    InterruptController::primary().end_of_interrupt(legacy_irq_number());
    s_scheduler_handler(register_state);
}

void Timer::discover_and_setup()
{
    // TODO: add HPET and maybe something else?
    new PIT;

    if (InterruptController::primary().matches_type(APIC::type)) {
        auto* lapic_timer = new LAPIC::Timer;
        lapic_timer->make_child_of(&InterruptController::primary());
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
    auto* primary = DeviceManager::the().primary_of(Category::TIMER);
    ASSERT(primary != nullptr);

    return *static_cast<Timer*>(primary);
}

Timer& Timer::get_specific(StringView type)
{
    auto timer = DeviceManager::the().one_of(Category::TIMER, type);

    if (timer)
        return *static_cast<Timer*>(timer);

    String error_string;
    error_string << "Failed to find timer of type " << type;
    runtime::panic(error_string.data());
}
}
