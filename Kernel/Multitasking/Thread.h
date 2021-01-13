#pragma once

#include "Common/List.h"
#include "Core/CPU.h"
#include "Interrupts/Timer.h"
#include "Memory/AddressSpace.h"
#include "TSS.h"

namespace kernel {

class Window;

class Thread : public StandaloneListNode {
    MAKE_NONCOPYABLE(Thread);
    MAKE_NONMOVABLE(Thread);

public:
    friend class Process;

    enum class State {
        RUNNING,
        DEAD,
        BLOCKED,
        READY,
    };

    struct PACKED ControlBlock {
        ptr_t current_kernel_stack_top;
    };

    static RefPtr<Thread> create_idle(Process& owner);
    static RefPtr<Thread> create_supervisor(Process& owner, Address kernel_stack, Address entrypoint);
    static RefPtr<Thread> create_user(Process& owner, Address kernel_stack, Address user_stack, Address entrypoint);

    void activate();
    void deactivate();

    [[nodiscard]] State state() const { return m_state; }

    static Thread* current() { return CPU::current().current_thread(); }

    [[nodiscard]] ControlBlock* control_block() { return &m_control_block; }

    [[nodiscard]] IsSupervisor is_supervisor() const { return m_is_supervisor; }

    [[nodiscard]] Process& owner() { return m_owner; }
    [[nodiscard]] AddressSpace& address_space();

    void sleep(u64 until)
    {
        m_state = State::BLOCKED;
        m_wake_up_time = until;
    }

    void exit() { m_state = State::DEAD; }

    void wake_up() { m_state = State::READY; }
    [[nodiscard]] u64 wakeup_time() const { return m_wake_up_time; }

    [[nodiscard]] bool is_sleeping() const { return m_state == State::BLOCKED; }
    [[nodiscard]] bool is_dead() const { return m_state == State::DEAD; }
    [[nodiscard]] bool is_running() const { return m_state == State::RUNNING; }
    [[nodiscard]] bool is_ready() const { return m_state == State::READY; }

    bool should_be_woken_up() const { return m_wake_up_time <= Timer::nanoseconds_since_boot(); }

    void set_window(Window* window) { m_window = window; }

    class WakeTimePtrComparator {
    public:
        bool operator()(const Thread* l, const Thread* r) const
        {
            ASSERT(l != nullptr);
            ASSERT(r != nullptr);

            return l->wakeup_time() < r->wakeup_time();
        }
    };

private:
    Thread(Process& owner, Address kernel_stack, IsSupervisor);

private:
    u32 m_id { 0 };

    Process& m_owner;

    Address m_initial_kernel_stack_top { nullptr };
    ControlBlock m_control_block { 0 };

    State m_state { State::READY };
    IsSupervisor m_is_supervisor { IsSupervisor::NO };

    u64 m_wake_up_time { 0 };

    Window* m_window { nullptr };
};
}
