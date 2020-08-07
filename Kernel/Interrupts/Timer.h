#pragma once

#include "Common/Lock.h"
#include "Common/String.h"
#include "Common/Types.h"

namespace kernel {

class Timer {
public:
    static constexpr u32 default_ticks_per_second   = 100;
    static constexpr u32 nanoseconds_in_microsecond = 1000;
    static constexpr u32 nanoseconds_in_millisecond = 1000000;
    static constexpr u32 nanoseconds_in_second      = 1000000000;
    static constexpr u32 milliseconds_in_second     = 1000;

    static void discover_and_setup();

    static Timer& the();

    virtual u64  nanoseconds_since_boot()       = 0;
    virtual void increment_time_since_boot(u64) = 0;

    virtual void set_frequency(u32 ticks_per_second) = 0;

    void lock(bool& interrupt_state) { m_lock.lock(interrupt_state); }
    void unlock(bool interrupt_state) { m_lock.unlock(interrupt_state); }

    virtual void nano_delay(u32) = 0;
    virtual void micro_delay(u32 us) { nano_delay(us * nanoseconds_in_microsecond); }
    virtual void mili_delay(u32 ms) { nano_delay(ms * nanoseconds_in_millisecond); }

    virtual u32 max_frequency() const = 0;

    virtual u32 current_frequency() const = 0;

    virtual StringView model() const          = 0;
    virtual bool       has_internal_counter() = 0;

    virtual void enable()  = 0;
    virtual void disable() = 0;

    virtual ~Timer() = default;

private:
    InterruptSafeSpinLock m_lock;
    static Timer*         s_timer;
};
}
