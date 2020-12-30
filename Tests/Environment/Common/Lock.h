#pragma once

#include "Atomic.h"

namespace kernel {

class InterruptSafeSpinLock
{
public:
    void lock(bool) {}
    void unlock(bool) {}
    bool is_deadlocked() { return false; }
    bool try_lock(bool) { return true; }
};

template <typename T>
class LockGuard
{
public:
    explicit LockGuard(const T&) {}
};

}
#define LOCK_GUARD(x) (void)x

