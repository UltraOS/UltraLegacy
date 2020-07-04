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

    void register_process(RefPtr<Process> process);

private:
    Scheduler() = default;

private:
    DynamicArray<RefPtr<Process>> m_processes;

    static Thread*    s_first_ready_to_run_thread;
    static Thread*    s_last_ready_to_run_thread;
    static Scheduler* s_instance;
};
}
