#pragma once

#include "Common/String.h"
#include "Common/Types.h"

namespace kernel {

class Timer {
public:
    static constexpr u32 default_ticks_per_second = 100;
    static constexpr u32 nanoseconds_in_second    = 1000000000;

    static void discover_and_setup();

    static Timer& the();

    static u64 nanoseconds_since_boot();

    virtual void set_frequency(u32 ticks_per_second) = 0;

    virtual u32 max_frequency() const = 0;

    virtual u32 current_frequency() const = 0;

    virtual StringView model() const = 0;

    virtual void enable()  = 0;
    virtual void disable() = 0;

    virtual ~Timer() = default;

protected:
    void increment();

private:
    static u64    s_nanoseconds_since_boot;
    static Timer* s_timer;
};
}
