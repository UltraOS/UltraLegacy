#pragma once

#include "Memory/PageDirectory.h"
#include "TSS.h"
#include "Interrupts/Timer.h"

namespace kernel {

using userland_entrypoint_t = void (*)();

// defined in Core/crt0.asm
extern "C" userland_entrypoint_t userland_entrypoint;

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
    create_user_thread(PageDirectory& page_dir, Address user_stack, Address kernel_stack, Address entrypoint);

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
        m_state = State::BLOCKED;
        m_wake_up_time = until;
    }

    bool is_sleeping() { return m_state == State::BLOCKED; }
    void wake_up() { m_state = State::READY; }

    bool should_be_woken_up() { return m_wake_up_time <= Timer::nanoseconds_since_boot(); }

private:
    struct iret_stack_frame {
        Address instruction_pointer;
        u32     code_selector;
        u32     eflags;
        Address stack_pointer;
        u32     data_selector;
    };

    struct task_switcher_task_frame {
        u32     ebp;
        u32     edi;
        u32     esi;
        u32     ebx;
        Address instruction_pointer;
    };

    struct supervisor_starting_stack_frame {
        task_switcher_task_frame switcher_frame;
    };

    struct userland_starting_stack_frame {
        task_switcher_task_frame switcher_frame;
        iret_stack_frame         interrupt_frame;
    };

    static_assert(sizeof(iret_stack_frame) == 20);
    static_assert(sizeof(task_switcher_task_frame) == 20);
    static_assert(sizeof(supervisor_starting_stack_frame) == 20);
    static_assert(sizeof(userland_starting_stack_frame) == 40);

    Thread(PageDirectory& page_dir, Address kernel_stack);

private:
    u32            m_thread_id;
    PageDirectory& m_page_directory;
    ControlBlock   m_control_block { nullptr };
    Address        m_initial_kernel_stack_top { nullptr };
    State          m_state { State::READY };
    u64            m_wake_up_time { 0 };
    bool           m_is_supervisor { false };
    Thread*        m_previous { nullptr };
    Thread*        m_next { nullptr };

    static Thread* s_current;
    static u32     s_next_thread_id;
    static TSS*    s_tss;
};
}
