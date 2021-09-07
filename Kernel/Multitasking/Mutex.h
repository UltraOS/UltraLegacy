#pragma once

#include "Blocker.h"
#include "Common/Lock.h"

namespace kernel {

class Thread;

class Mutex {
public:
    void lock();
    void unlock();

private:
    InterruptSafeSpinLock m_state_access_lock;
    Thread* m_owner { nullptr };

    List<MutexBlocker> m_waiters;
};

class SharedMutex {
public:
    void exclusive_lock();
    void exclusive_unlock();

    void shared_lock();
    void shared_unlock();

private:
    InterruptSafeSpinLock m_state_access_lock;

    static constexpr ssize_t exclusive_mode = -1;
    static constexpr ssize_t free_mode = 0;

    // 0 - not owned by anyone
    // < 0 - exclusive ownership
    // > 0 - shared ownership of N
    ssize_t m_shared_refcount { 0 };

    List<MutexBlocker> m_shared_waiters;
    List<MutexBlocker> m_exclusive_waiters;
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
