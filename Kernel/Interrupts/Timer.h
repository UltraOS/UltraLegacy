#pragma once

#include "Common/Lock.h"
#include "Common/String.h"
#include "Common/Types.h"

#include "Time/Time.h"

namespace kernel {

class Timer {
    MAKE_INHERITABLE_SINGLETON(Timer) = default;

public:
    static constexpr u32 default_ticks_per_second = 100;

    static void discover_and_setup();

    static Timer& the();

    virtual u64 nanoseconds_since_boot() = 0;
    virtual void increment_time_since_boot(u64) = 0;

    virtual void set_frequency(u32 ticks_per_second) = 0;

    InterruptSafeSpinLock& timer_lock() { return m_lock; }

    virtual void nano_delay(u32) = 0;
    virtual void micro_delay(u32 us) { nano_delay(us * Time::nanoseconds_in_microsecond); }
    virtual void mili_delay(u32 ms) { nano_delay(ms * Time::nanoseconds_in_millisecond); }

    virtual u32 max_frequency() const = 0;

    virtual u32 current_frequency() const = 0;

    virtual StringView model() const = 0;
    virtual bool has_internal_counter() = 0;

    virtual void enable() = 0;
    virtual void disable() = 0;

    virtual ~Timer() = default;

private:
    InterruptSafeSpinLock m_lock;
    static Timer* s_timer;
};
}
