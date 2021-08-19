#pragma once

#include "Time/Time.h"
#include "Interrupts/Timer.h"

namespace kernel {

struct WaitResult {
    bool success;
    size_t elapsed_ms;
};

template <typename Callback>
static WaitResult repeat_until(Callback cb, size_t timeout_ms)
{
    u64 wait_begin = Timer::nanoseconds_since_boot();
    u64 current_time = wait_begin;
    u64 wait_end = current_time + timeout_ms * Time::nanoseconds_in_millisecond;

    while (current_time <= wait_end)
    {
        if (cb())
            return { true, static_cast<size_t>((current_time - wait_begin) / Time::nanoseconds_in_millisecond) };

        current_time = Timer::nanoseconds_since_boot();
        pause();
    }

    return { cb(), timeout_ms };
}

}
