#pragma once

namespace kernel {

class Thread;

class Blocker {
public:
    enum class Type {
        DISK_IO,
    };

    Blocker(Thread& blocked_thread, Type type);
    Type type() const { return m_type; }

    bool block();
    void unblock();
    virtual bool is_cancellable() { return true; }

    virtual ~Blocker() = default;

private:
    Thread& m_thread;
    Type m_type;
};

class DiskIOBlocker : public Blocker {
public:
    DiskIOBlocker(Thread& blocked_thread);

    virtual bool is_cancellable() { return false; }
};

}