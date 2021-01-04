#pragma once

#include "Common/Lock.h"
#include "Common/String.h"
#include "Common/Types.h"
#include "Common/Logger.h"

#include "Interrupts/IRQHandler.h"

#include "Time/Time.h"


namespace kernel {

class Timer : public IRQHandler {
public:
    explicit Timer(u16 irq_index);

    enum class Type {
        PIT,
        HPET,
        LAPIC,
        ACPI,
    };

    static constexpr u32 default_ticks_per_second = 100;

    using SchedulerEventHandler = void(*)(const RegisterState&);
    using TransparentEventHandler = void(*)();

    static void discover_and_setup();
    
    static void register_scheduler_handler(SchedulerEventHandler);
    static void register_handler(TransparentEventHandler);
    static void unregsiter_handler(TransparentEventHandler);
    
    static Timer& primary();
    static Timer& get_specific(Type);

    void make_primary();
    bool is_primary() const;

    virtual void calibrate_for_this_processor() { }

    static u64 nanoseconds_since_boot() { return s_nanoseconds_since_boot; }

    virtual void set_frequency(u32 ticks_per_second) = 0;

    InterruptSafeSpinLock& lock() { return m_timer_specific_lock; }

    virtual void nano_delay(u32) = 0;
    virtual void micro_delay(u32 us) { nano_delay(us * Time::nanoseconds_in_microsecond); }
    virtual void mili_delay(u32 ms) { nano_delay(ms * Time::nanoseconds_in_millisecond); }

    virtual u32 max_frequency() const = 0;
    virtual u32 current_frequency() const = 0;

    virtual bool is_per_cpu() const = 0;
    virtual StringView model() const = 0;
    virtual Type type() const = 0;

    virtual void enable() = 0;
    virtual void disable() = 0;

    virtual ~Timer()
    {
        LOCK_GUARD(s_lock);

        auto this_timer = linear_search(s_timers.begin(), s_timers.end(), this);
        ASSERT(this_timer != s_timers.end());

        s_timers.erase(this_timer);

        if (!is_primary())
            return;

        bool did_replace_timer = false;

        for (auto timer : s_timers) {
            if (timer->is_primary())
                continue;

            timer->make_primary();
            did_replace_timer = true;
            break;
        }
        
        if (!did_replace_timer)
            runtime::panic("Failed to find a replacement for current timer, cannot run without a timer");
    }

protected:
    void finalize_irq() override
    {
        InterruptController::the().end_of_interrupt(irq_index());
    }

private:
    void handle_irq(const RegisterState& registers) override
    {
        on_tick(registers, CPU::current().is_bsp());
    }

    void on_tick(const RegisterState&, bool is_bsp);

private:
    InterruptSafeSpinLock m_timer_specific_lock;

    static Atomic<u64> s_nanoseconds_since_boot;
    static InterruptSafeSpinLock s_lock;
    static SchedulerEventHandler s_scheduler_handler;
    static DynamicArray<TransparentEventHandler> s_transparent_handlers;
    static Atomic<Timer*> s_primary_timer;
    static DynamicArray<Timer*> s_timers;
};
}
