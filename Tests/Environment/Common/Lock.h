#pragma once

namespace kernel {

class InterruptSafeSpinLock
{
public:
    void lock(bool) {}
    void unlock(bool) {}
};

template <typename T>
class LockGuard
{
public:
    explicit LockGuard(const T&) {}
};
}
