#pragma once

#include "Atomic.h"
#include "Interrupts/Utilities.h"

namespace kernel {

class SpinLock {
public:
    using lock_t = size_t;

    static constexpr lock_t unlocked = 0;
    static constexpr lock_t locked   = 1;

    void lock()
    {
        lock_t expected = unlocked;

        while (!m_lock.compare_and_exchange(&expected, locked)) {
            expected = unlocked;
            asm("pause");
        }
    }

    void unlock() { m_lock.store(unlocked, MemoryOrder::RELEASE); }

private:
    Atomic<size_t> m_lock;
};

class InterruptSafeSpinLock : SpinLock {
public:
    InterruptSafeSpinLock() { }

    void lock(bool& interrupt_state)
    {
        interrupt_state = Interrupts::are_enabled();
        Interrupts::disable();

        SpinLock::lock();
    }

    void unlock(bool interrupt_state)
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

    void lock()
    {
        auto this_cpu = CPU::current().id();

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

    void unlock()
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

    void lock(bool& interrupt_state)
    {
        interrupt_state = Interrupts::are_enabled();
        Interrupts::disable();

        RecursiveSpinLock::lock();
    }

    void unlock(bool interrupt_state)
    {
        RecursiveSpinLock::unlock();

        if (interrupt_state)
            Interrupts::enable();
    }
};

class ScopedInterruptSafeLock {
public:
    ScopedInterruptSafeLock(SpinLock& lock) : m_lock(lock) { this->lock(); }
    ~ScopedInterruptSafeLock() { unlock(); }

private:
    void lock()
    {
        m_should_enable_interrupts = Interrupts::are_enabled();
        Interrupts::disable();

        m_lock.lock();
    }

    void unlock()
    {
        m_lock.unlock();

        if (m_should_enable_interrupts)
            Interrupts::enable();
    }

private:
    bool      m_should_enable_interrupts { false };
    SpinLock& m_lock;
};
}
