#pragma once

#include "Common/List.h"
#include "Core/CPU.h"
#include "Interrupts/Timer.h"
#include "Memory/AddressSpace.h"
#include "TSS.h"
#include "TaskLoader.h"
#include "WindowManager/Window.h"

#include "Blocker.h"

namespace kernel {

class Thread : public StandaloneListNode<Thread> {
    MAKE_NONCOPYABLE(Thread);
    MAKE_NONMOVABLE(Thread);

public:
    friend class Process;

    enum class State {
        UNDEFINED,
        RUNNING,
        DEAD,
        BLOCKED,
        READY,
    };

    struct PACKED ControlBlock {
        ptr_t current_kernel_stack_top;
    };

    static RefPtr<Thread> create_idle(Process& owner);
    static RefPtr<Thread> create_supervisor(Process& owner, RefPtr<VirtualRegion> kernel_stack, Address entrypoint);
    static RefPtr<Thread> create_user(
        Process& owner,
        RefPtr<VirtualRegion> kernel_stack,
        TaskLoader::LoadRequest*);

    void activate();
    void deactivate();

    [[nodiscard]] State state() const { return m_state; }

    void* fpu_state() const { return m_fpu_state; }

    static Thread* current()
    {
        Interrupts::ScopedDisabler d;
        return CPU::current().current_thread();
    }

    [[nodiscard]] ControlBlock* control_block() { return &m_control_block; }

    [[nodiscard]] IsSupervisor is_supervisor() const { return m_is_supervisor; }

    [[nodiscard]] Process& owner() { return m_owner; }
    [[nodiscard]] AddressSpace& address_space();

    [[nodiscard]] bool is_main() const;

    [[nodiscard]] VirtualRegion& kernel_stack()
    {
        ASSERT(!m_kernel_stack.is_null());
        return *m_kernel_stack;
    }

    bool should_die() const { return m_should_die.load(MemoryOrder::ACQUIRE); }
    bool is_invulnerable() const { return m_is_invulnerable; }
    void set_invulnerable(bool value) { m_is_invulnerable = value; }

    bool request_state(State new_state, State expected = State::UNDEFINED)
    {
        return m_requested_state.compare_and_exchange(&expected, new_state);
    }

    State requested_state() { return m_requested_state.load(MemoryOrder::ACQUIRE); }

    class ScopedInvulnerability {
    public:
        ScopedInvulnerability(Thread& thread = *current())
            : m_thread(thread)
            , m_previous_value(thread.is_invulnerable())
        {
            thread.set_invulnerable(true);
        }

        ~ScopedInvulnerability()
        {
            m_thread.set_invulnerable(m_previous_value);
        }

    private:
        Thread& m_thread;
        bool m_previous_value { false };
    };

    Map<u32, RefPtr<Window>>& windows() { return m_windows; }

    u32 add_window(RefPtr<Window> window)
    {
        auto id = m_window_ids.fetch_add(1, MemoryOrder::ACQ_REL) + 1;
        m_windows[id] = window;

        return id;
    }

    RefPtr<Window> window_at_id(u32 id)
    {
        auto window = m_windows.find(id);

        if (window != m_windows.end())
            return window->second;

        return {};
    }

    // A bunch of operators to make RedBlackTree work with Thread
    friend bool operator<(const RefPtr<Thread>& l, const RefPtr<Thread>& r)
    {
        return l->m_id < r->m_id;
    }

    friend bool operator<(const RefPtr<Thread>& l, u32 id)
    {
        return l->m_id < id;
    }

    friend bool operator<(u32 id, const RefPtr<Thread>& r)
    {
        return id < r->m_id;
    }

    [[nodiscard]] u32 id() const { return m_id; }
    [[nodiscard]] i32 exit_code() const { return m_exit_code; }

private:
    Thread(Process& owner);
    Thread(Process& owner, RefPtr<VirtualRegion> kernel_stack, IsSupervisor);

    friend class Scheduler;
    void block(Blocker* blocker)
    {
        ASSERT(m_blocker == nullptr);
        ASSERT(blocker != nullptr);
        set_blocker(blocker);
        request_state(Thread::State::BLOCKED);
    }

    void kill_all_threads_on_exit() { m_should_kill_all_threads = true; }
    [[nodiscard]] bool should_kill_all_threads_on_exit() const { return m_should_kill_all_threads; }

    // NOTE: Important invariant
    // All member functions in the following block must only be called with Scheduler::s_queues_lock held.
    // ---------------------------------------------------------------------------------------------------
    void unblock();
    bool interrupt();
    void exit(i32 code)
    {
        m_should_die.store(true, MemoryOrder::RELEASE);
        m_exit_code = code;
    }

    [[nodiscard]] bool is_blocked() const { return m_state == State::BLOCKED; }
    [[nodiscard]] bool is_dead() const { return m_state == State::DEAD; }
    [[nodiscard]] bool is_running() const { return m_state == State::RUNNING; }
    [[nodiscard]] bool is_ready() const { return m_state == State::READY; }
    void set_state(State state) { m_state = state; }

    StringView state_to_string()
    {
        switch (state()) {
        case State::UNDEFINED:
            return "UNDEFINED"_sv;
        case State::RUNNING:
            return "RUNNING"_sv;
        case State::DEAD:
            return "DEAD"_sv;
        case State::BLOCKED:
            return "BLOCKED"_sv;
        case State::READY:
            return "READY"_sv;
        default:
            return "INVALID"_sv;
        }
    }

    void set_blocker(Blocker* blocker) { m_blocker = blocker; }
    Blocker* blocker() const { return m_blocker; }
    // ---------------------------------------------------------------------------------------------------

private:
    u32 m_id { 0 };
    i32 m_exit_code { 0 };

    Process& m_owner;

    RefPtr<VirtualRegion> m_kernel_stack;

    ControlBlock m_control_block { 0 };

    State m_state { State::UNDEFINED };
    IsSupervisor m_is_supervisor { IsSupervisor::NO };

    Blocker* m_blocker { nullptr };

    void* m_fpu_state { nullptr };

    Atomic<State> m_requested_state { State::UNDEFINED };
    Atomic<bool> m_should_die { false };
    bool m_is_invulnerable { false };
    bool m_should_kill_all_threads { false };

    Atomic<u32> m_window_ids { 0 };
    Map<u32, RefPtr<Window>> m_windows;
};
}
