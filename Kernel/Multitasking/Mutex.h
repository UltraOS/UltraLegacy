#pragma once

#include "Blocker.h"
#include "Common/Lock.h"
#include "Thread.h"

namespace kernel {

class Mutex {
public:
    void lock();
    void unlock();

private:
    InterruptSafeSpinLock m_state_access_lock;
    Thread* m_owner { nullptr };

    List<MutexBlocker> m_waiters;
};

template <>
class LockGuard<Mutex> {
public:
    LockGuard(Mutex& lock, const char* = nullptr, size_t = 0)
        : m_lock(lock)
    {
        m_lock.lock();
    }

    ~LockGuard() { m_lock.unlock(); }

private:
    Mutex& m_lock;
};

}