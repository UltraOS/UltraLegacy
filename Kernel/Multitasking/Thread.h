#pragma once

#include "Core/CPU.h"
#include "Interrupts/Timer.h"
#include "Memory/AddressSpace.h"
#include "TSS.h"

namespace kernel {

using entrypoint_t = void (*)();

// defined in Core/crt0.asm
extern "C" entrypoint_t user_thread_entrypoint;
extern "C" entrypoint_t supervisor_thread_entrypoint;

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

    static Thread* current() { return s_current; }

    ControlBlock* control_block() { return &m_control_block; }

    bool is_supervisor() const { return m_is_supervisor; }
    bool is_user() const { return !is_supervisor(); }

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

    bool is_sleeping() { return m_state == State::BLOCKED; }
    void wake_up() { m_state = State::READY; }

    bool is_dead() { return m_state == State::DEAD; }

    bool should_be_woken_up() { return m_wake_up_time <= Timer::nanoseconds_since_boot(); }

private:
    struct supervisor_iret_frame {
        Address    instruction_pointer;
        u32        code_selector;
        CPU::FLAGS eflags;
    };

    struct user_iret_stack_frame {
        Address    instruction_pointer;
        u32        code_selector;
        CPU::FLAGS eflags;
        Address    stack_pointer;
        u32        data_selector;
    };

    struct task_switcher_stack_frame {
        u32     ebp;
        u32     edi;
        u32     esi;
        u32     ebx;
        Address instruction_pointer;
    };

    struct supervisor_thread_stack_frame {
        task_switcher_stack_frame switcher_frame;
        supervisor_iret_frame     iret_frame;
    };

    struct user_thread_stack_frame {
        task_switcher_stack_frame switcher_frame;
        user_iret_stack_frame     iret_frame;
    };

    Thread(AddressSpace& page_dir, Address kernel_stack);

private:
    u32           m_thread_id;
    AddressSpace& m_page_directory;
    ControlBlock  m_control_block { nullptr };
    Address       m_initial_kernel_stack_top { nullptr };
    State         m_state { State::READY };
    u64           m_wake_up_time { 0 };
    bool          m_is_supervisor { false };
    u8            m_exit_code { 0 };
    Thread*       m_previous { nullptr };
    Thread*       m_next { nullptr };

    static Thread* s_current;
    static u32     s_next_thread_id;
    static TSS*    s_tss;
};
}
