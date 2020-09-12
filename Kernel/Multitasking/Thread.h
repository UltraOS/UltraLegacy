#pragma once

#include "Core/CPU.h"
#include "Interrupts/Timer.h"
#include "Memory/AddressSpace.h"
#include "TSS.h"

namespace kernel {

class Window;

class Thread {
public:
    friend class Process;

    enum class State {
        RUNNING,
        DEAD,
        BLOCKED,
        READY,
    };

    struct ControlBlock {
        Address current_kernel_stack_top;
    };

    static void initialize();

    static RefPtr<Thread> create_supervisor_thread(Address kernel_stack, Address entrypoint);

    static RefPtr<Thread>
    create_user_thread(AddressSpace& page_dir, Address user_stack, Address kernel_stack, Address entrypoint);

    void    set_next(Thread* thread) { m_next = thread; }
    Thread* next() { return m_next; }
    bool    has_next() { return next(); }

    void    set_previous(Thread* thread) { m_previous = thread; }
    Thread* previous() { return m_previous; }
    bool    has_previous() { return previous(); }

    void activate();
    void deactivate();

    State state() const { return m_state; }

    static Thread* current() { return CPU::current().current_thread(); }

    ControlBlock* control_block() { return &m_control_block; }

    bool is_supervisor() const { return m_is_supervisor; }
    bool is_user() const { return !is_supervisor(); }

    AddressSpace& address_space() { return m_address_space; }

    void sleep(u64 until)
    {
        m_state        = State::BLOCKED;
        m_wake_up_time = until;
    }

    void exit(u8 code)
    {
        m_state     = State::DEAD;
        m_exit_code = code;
    }

    bool is_sleeping() const { return m_state == State::BLOCKED; }
    void wake_up() { m_state = State::READY; }

    bool is_dead() const { return m_state == State::DEAD; }
    bool is_running() const { return m_state == State::RUNNING; }
    bool is_ready() const { return m_state == State::READY; }

    bool should_be_woken_up() const { return m_wake_up_time <= Timer::the().nanoseconds_since_boot(); }

    void set_window(Window* window) { m_window = window; }

private:
    Thread(AddressSpace& page_dir, Address kernel_stack);

private:
    u32           m_thread_id;
    AddressSpace& m_address_space;
    ControlBlock  m_control_block { nullptr };
    Address       m_initial_kernel_stack_top { nullptr };
    State         m_state { State::READY };
    u64           m_wake_up_time { 0 };
    bool          m_is_supervisor { false };
    u8            m_exit_code { 0 };
    Thread*       m_previous { nullptr };
    Thread*       m_next { nullptr };

    Window* m_window;

    static Atomic<u32> s_next_thread_id;
};
}
