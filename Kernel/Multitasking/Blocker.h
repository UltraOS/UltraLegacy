#pragma once

#include "Common/Atomic.h"
#include "Common/List.h"

namespace kernel {

class Thread;

class Blocker {
public:
    enum class Type {
        DISK_IO,
        MUTEX
    };

    enum class Result {
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

    virtual bool is_interruptable() { return false; }
};

class MutexBlocker : public Blocker, public StandaloneListNode<MutexBlocker> {
public:
    MutexBlocker(Thread& blocked_thread);
};

}