#pragma once

#include "Common/Types.h"
#include "Interrupts/Common.h"
#include "Process.h"
#include "Thread.h"

namespace kernel {

class Scheduler {
public:
    static void       inititalize();
    static Scheduler& the();

    static void on_tick(const RegisterState&);

    static void yield();

    static void pick_next();

    static void switch_task(Thread::ControlBlock* current_task, Thread::ControlBlock* new_task);

    static void enqueue_thread(Thread& thread);
    static void enqueue_sleeping_thread(Thread& thread);

    static void wake_up_ready_threads();

    void register_process(RefPtr<Process> process);

private:
    Scheduler() = default;

private:
    DynamicArray<RefPtr<Process>> m_processes;

    static Thread*    s_sleeping_threads;
    static Scheduler* s_instance;
};
}
