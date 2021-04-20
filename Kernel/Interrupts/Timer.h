#pragma once

#include "Common/Lock.h"
#include "Common/Logger.h"
#include "Common/String.h"
#include "Common/Types.h"

#include "Drivers/Device.h"
#include "Drivers/DeviceManager.h"
#include "Interrupts/IRQHandler.h"

#include "Time/Time.h"

namespace kernel {

class Timer : public IRQHandler, public Device {
public:
    explicit Timer(IRQHandler::Type, u16 irq_index = any_vector);

    static constexpr u32 default_ticks_per_second = 100;

    using SchedulerEventHandler = void (*)(const RegisterState&);
    using TransparentEventHandler = void (*)();

    static void discover_and_setup();

    static void register_scheduler_handler(SchedulerEventHandler);
    static void register_handler(TransparentEventHandler);
    static void unregsiter_handler(TransparentEventHandler);

    static Timer& primary();
    static Timer& get_specific(StringView);

    virtual void calibrate_for_this_processor() { }

    static u64 nanoseconds_since_boot() { return s_nanoseconds_since_boot; }

    virtual void set_frequency(u32 ticks_per_second) = 0;

    InterruptSafeSpinLock& lock() { return m_timer_specific_lock; }

    virtual size_t minimum_delay_ns() const = 0;
    virtual size_t maximum_delay_ns() const = 0;

    virtual void nano_delay(u32) = 0;
    virtual void micro_delay(u32 us) { nano_delay(us * Time::nanoseconds_in_microsecond); }
    virtual void mili_delay(u32 ms) { nano_delay(ms * Time::nanoseconds_in_millisecond); }

    virtual u32 max_frequency() const = 0;
    virtual u32 current_frequency() const = 0;

    virtual bool is_per_cpu() const = 0;

    virtual void enable() = 0;
    virtual void disable() = 0;

private:
    bool handle_irq(RegisterState& registers)
    {
        on_tick(registers, CPU::current().is_bsp());
        return true;
    }

    void on_tick(const RegisterState&, bool is_bsp);

private:
    InterruptSafeSpinLock m_timer_specific_lock;

    static Atomic<u64> s_nanoseconds_since_boot;
    static InterruptSafeSpinLock s_lock;
    static SchedulerEventHandler s_scheduler_handler;
    static DynamicArray<TransparentEventHandler> s_transparent_handlers;
};
}
