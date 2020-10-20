#pragma once

#include "Atomic.h"
#include "Interrupts/Utilities.h"

namespace kernel {

class SpinLock {
    MAKE_NONCOPYABLE(SpinLock);

public:
    SpinLock() = default;

    using lock_t = size_t;

    static constexpr lock_t unlocked = 0;
    static constexpr lock_t locked   = 1;

    void lock() ALWAYS_INLINE
    {
        lock_t expected = unlocked;

        while (!m_lock.compare_and_exchange(&expected, locked)) {
            expected = unlocked;
            pause();
        }
    }

    void unlock() ALWAYS_INLINE { m_lock.store(unlocked, MemoryOrder::RELEASE); }

private:
    Atomic<size_t> m_lock;
};

class InterruptSafeSpinLock : SpinLock {
public:
    InterruptSafeSpinLock() { }

    void lock(bool& interrupt_state) ALWAYS_INLINE
    {
        interrupt_state = Interrupts::are_enabled();
        Interrupts::disable();

        SpinLock::lock();
    }

    void unlock(bool interrupt_state) ALWAYS_INLINE
    {
        SpinLock::unlock();

        if (interrupt_state)
            Interrupts::enable();
    }
};

class RecursiveSpinLock {
public:
    using lock_t = size_t;

    static constexpr lock_t unlocked = -1;

    void lock() ALWAYS_INLINE
    {
        u32 this_cpu;

        // a hack to make recursive spinlock work before initializing the APIC
        if (CPU::is_initialized())
            this_cpu = CPU::current().id();
        else
            this_cpu = 0;

        lock_t expected = unlocked;

        while (!m_lock.compare_and_exchange(&expected, this_cpu)) {
            if (expected == this_cpu) {
                ++m_depth;
                return; // we already own the lock
            }

            expected = unlocked;
            asm("pause");
        }

        ++m_depth;
    }

    void unlock() ALWAYS_INLINE
    {
        ASSERT(m_depth > 0);

        if (--m_depth == 0)
            m_lock.store(unlocked, MemoryOrder::RELEASE);
    }

    size_t depth() const { return m_depth; }

private:
    size_t         m_depth { 0 };
    Atomic<size_t> m_lock { unlocked };
};

class RecursiveInterruptSafeSpinLock : public RecursiveSpinLock {
public:
    RecursiveInterruptSafeSpinLock() { }

    void lock(bool& interrupt_state) ALWAYS_INLINE
    {
        interrupt_state = Interrupts::are_enabled();
        Interrupts::disable();

        RecursiveSpinLock::lock();
    }

    void unlock(bool interrupt_state) ALWAYS_INLINE
    {
        RecursiveSpinLock::unlock();

        if (interrupt_state)
            Interrupts::enable();
    }
};

template <typename LockT>
class LockGuard {
public:
    LockGuard(LockT& lock) : m_lock(lock) { this->lock(); }
    ~LockGuard() { unlock(); }

private:
    void lock() ALWAYS_INLINE { m_lock.lock(); }

    void unlock() ALWAYS_INLINE { m_lock.unlock(); }

private:
    RecursiveSpinLock& m_lock;
};

template <>
class LockGuard<RecursiveInterruptSafeSpinLock> {
public:
    LockGuard(RecursiveInterruptSafeSpinLock& lock) : m_lock(lock) { this->lock(); }
    ~LockGuard() { unlock(); }

private:
    void lock() ALWAYS_INLINE { m_lock.lock(m_state); }

    void unlock() ALWAYS_INLINE { m_lock.unlock(m_state); }

private:
    bool                            m_state { false };
    RecursiveInterruptSafeSpinLock& m_lock;
};

template <>
class LockGuard<InterruptSafeSpinLock> {
public:
    LockGuard(InterruptSafeSpinLock& lock) : m_lock(lock) { this->lock(); }
    ~LockGuard() { unlock(); }

private:
    void lock() ALWAYS_INLINE { m_lock.lock(m_state); }

    void unlock() ALWAYS_INLINE { m_lock.unlock(m_state); }

private:
    bool                   m_state { false };
    InterruptSafeSpinLock& m_lock;
};
}
