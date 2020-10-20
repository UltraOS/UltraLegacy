#pragma once

#include "Common/Types.h"

#include "Core/Registers.h"

#include "Process.h"
#include "Thread.h"

namespace kernel {

class Scheduler {
    MAKE_SINGLETON(Scheduler) = default;

public:
    static void       inititalize();
    static Scheduler& the();

    static void schedule(const RegisterState* registers);

    static void on_tick(const RegisterState&);

    static void yield();

    static void pick_next();

    static void switch_task(Thread::ControlBlock* new_task);

    static void save_state_and_schedule();

    static void enqueue_thread(Thread& thread);
    static void dequeue_thread(Thread& thread);

    static void enqueue_sleeping_thread(Thread& thread);

    static void wake_up_ready_threads();

    static Thread* last_picked() { return s_last_picked; }
    static void    set_last_picked(Thread* thread) { s_last_picked = thread; }

    void register_process(RefPtr<Process> process);

private:
    DynamicArray<RefPtr<Process>> m_processes;

    static RecursiveInterruptSafeSpinLock s_lock;

    static size_t     s_thread_count;
    static Thread*    s_last_picked;
    static Thread*    s_sleeping_threads;
    static Scheduler* s_instance;
};
}
