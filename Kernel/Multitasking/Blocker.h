#pragma once

#include "Common/Atomic.h"
#include "Common/List.h"

namespace kernel {

class Thread;

class Blocker {
public:
    enum class Type : u32 {
        DISK_IO,
        MUTEX,
        IO,
        PROCESS_LOAD,
        SLEEP,
        IRQ
    };

    enum class Result : u32 {
        UNDEFINED,
        UNBLOCKED,
        INTERRUPTED,
        TIMEOUT
    };

    Blocker(Thread& blocked_thread, Type type);
    Type type() const { return m_type; }

    Result block();

    void unblock();
    void set_result(Result);
    bool should_block() { return m_should_block; }

    Thread& thread() { return m_thread; }

    virtual bool is_interruptable() { return true; }

    virtual ~Blocker() = default;

private:
    Thread& m_thread;
    Type m_type;
    Atomic<Result> m_result { Result::UNDEFINED };
    bool m_should_block { true };
};

class DiskIOBlocker : public Blocker {
public:
    DiskIOBlocker(Thread& blocked_thread);

    bool is_interruptable() override { return false; }
};

class MutexBlocker : public Blocker, public StandaloneListNode<MutexBlocker> {
public:
    MutexBlocker(Thread& blocked_thread);

    // Technically it should be interruptable, but we would need to handle lock
    // errors everywhere, and we just don't have that yet.
    bool is_interruptable() override { return false; }
};

class ProcessLoadBlocker : public Blocker {
public:
    ProcessLoadBlocker(Thread& blocked_thread);

    bool is_interruptable() override { return false; }
};

class IOBlocker : public Blocker, public StandaloneListNode<IOBlocker> {
public:
    IOBlocker(Thread& blocked_thread);
};

class SleepBlocker : public Blocker {
public:
    SleepBlocker(Thread& blocked_thread, u64 wake_time);

    class WakeTimePtrComparator {
    public:
        bool operator()(const SleepBlocker* l, const SleepBlocker* r) const
        {
            ASSERT(l != nullptr);
            ASSERT(r != nullptr);

            return l->wakeup_time() < r->wakeup_time();
        }
    };

    [[nodiscard]] u64 wakeup_time() const { return m_wake_time; }
    [[nodiscard]] bool should_be_woken_up() const;

private:
    u64 m_wake_time { 0 };
};

class IRQBlocker : public Blocker {
public:
    IRQBlocker(Thread& blocked_thread);
};

}
