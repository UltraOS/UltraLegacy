#pragma once

#include "Atomic.h"
#include "Interrupts/Utilities.h"

namespace kernel {

class DeadlockWatcher {
protected:
    void did_acquire_lock(StringView file, size_t line, size_t core_id) ALWAYS_INLINE
    {
        m_acquire_attempts = 0;
        m_file = file;
        m_line = line;
        m_core = core_id;
        m_backtrace_frames = runtime::dump_backtrace(m_backtrace, max_watcher_depth);
    }

    void did_fail_to_acquire() ALWAYS_INLINE
    {
        static constexpr size_t max_acquire_attempts = 10000000;

        ++m_acquire_attempts;

        if (m_acquire_attempts >= max_acquire_attempts)
            did_deadlock();
    }

private:
    void did_deadlock()
    {
        String deadlock_message;

        deadlock_message << "Deadlock on cpu " << CPU::current_id()
                         << "! Last acquired by cpu " << m_core
                         << " at " << m_file << ":" << m_line
                         << "\nLast acquirer backtrace:\n";

        if (m_backtrace_frames == 0)
            deadlock_message << "No backtrace information (possible stack corruption)";

        for (size_t i = 0; i < m_backtrace_frames; ++i) {
            auto* symbol = runtime::KernelSymbolTable::find_symbol(m_backtrace[i]);
            auto* name = symbol ? symbol->name() : "??";

            deadlock_message << "Frame " << i << ": " << Address(m_backtrace[i]) << " in " << name << "\n";
        }

        runtime::panic(deadlock_message.c_string());
    }

private:
    size_t m_acquire_attempts { 0 };

    static constexpr size_t max_watcher_depth = 4;
    StringView m_file;
    size_t m_line { 0 };
    size_t m_core { 0 };
    size_t m_backtrace_frames { 0 };
    ptr_t m_backtrace[max_watcher_depth];
};

class SpinLock : DeadlockWatcher {
    MAKE_NONCOPYABLE(SpinLock);

public:
    SpinLock() = default;

    using lock_t = size_t;

    static constexpr lock_t unlocked = 0;
    static constexpr lock_t locked = 1;

    void lock(const char* file = nullptr, size_t line = 0, size_t core_id = 0) ALWAYS_INLINE
    {
        if (!file)
            file = "<unspecified>";

        lock_t expected = unlocked;

        while (!m_lock.compare_and_exchange(&expected, locked)) {
            did_fail_to_acquire();

            expected = unlocked;
            pause();
        }

        did_acquire_lock(file, line, core_id);
    }

    void unlock() ALWAYS_INLINE { m_lock.store(unlocked, MemoryOrder::RELEASE); }

private:
    Atomic<size_t> m_lock;
};

class InterruptSafeSpinLock : SpinLock {
public:
    InterruptSafeSpinLock() = default;

    void lock(bool& interrupt_state, const char* file = nullptr, size_t line = 0, size_t core_id = 0) ALWAYS_INLINE
    {
        interrupt_state = Interrupts::are_enabled();
        Interrupts::disable();

        SpinLock::lock(file, line, core_id);
    }

    void unlock(bool interrupt_state) ALWAYS_INLINE
    {
        SpinLock::unlock();

        if (interrupt_state)
            Interrupts::enable();
    }
};

class RecursiveSpinLock : public DeadlockWatcher {
public:
    using lock_t = size_t;

    static constexpr lock_t unlocked = -1;

    void lock(const char* file = nullptr, size_t line = 0) ALWAYS_INLINE
    {
        if (!file)
            file = "<unspecified>";

        u32 this_cpu = CPU::current_id();

        lock_t expected = unlocked;

        while (!m_lock.compare_and_exchange(&expected, this_cpu)) {
            if (expected == this_cpu) {
                ++m_depth;
                return; // we already own the lock
            }

            did_fail_to_acquire();

            expected = unlocked;
            asm("pause");
        }

        did_acquire_lock(file, line, this_cpu);

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
    size_t m_depth { 0 };
    Atomic<size_t> m_lock { unlocked };
};

class RecursiveInterruptSafeSpinLock : public RecursiveSpinLock {
public:
    RecursiveInterruptSafeSpinLock() = default;

    void lock(bool& interrupt_state, const char* file = nullptr, size_t line = 0) ALWAYS_INLINE
    {
        interrupt_state = Interrupts::are_enabled();
        Interrupts::disable();

        RecursiveSpinLock::lock(file, line);
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
    ALWAYS_INLINE LockGuard(LockT& lock, const char* file = nullptr, size_t line = 0)
        : m_lock(lock)
    {
        m_lock.lock(file, line);
    }

    ~LockGuard() ALWAYS_INLINE { m_lock.unlock(); }

private:
    RecursiveSpinLock& m_lock;
};

template <>
class LockGuard<RecursiveInterruptSafeSpinLock> {
public:
    ALWAYS_INLINE LockGuard(RecursiveInterruptSafeSpinLock& lock, const char* file = nullptr, size_t line = 0)
        : m_lock(lock)
    {
        m_lock.lock(m_state, file, line);
    }

    ~LockGuard() ALWAYS_INLINE { m_lock.unlock(m_state); }

private:
    bool m_state { false };
    RecursiveInterruptSafeSpinLock& m_lock;
};

template <>
class LockGuard<InterruptSafeSpinLock> {
public:
    ALWAYS_INLINE LockGuard(InterruptSafeSpinLock& lock, const char* file = nullptr, size_t line = 0)
        : m_lock(lock)
    {
        m_lock.lock(m_state, file, line, CPU::current_id());
    }

    ~LockGuard() ALWAYS_INLINE { m_lock.unlock(m_state); }

private:
    bool m_state { false };
    InterruptSafeSpinLock& m_lock;
};
}

#define LOCK_GUARD(lock) LockGuard<remove_reference_t<decltype(lock)>> lock_guard(lock, __FILE__, __LINE__)
