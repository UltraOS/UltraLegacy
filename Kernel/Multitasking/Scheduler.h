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

    static void switch_task(Thread::ControlBlock* current_task, Thread::ControlBlock* new_task);

    static Thread& current_thread();

    void enqueue_thread(Thread& thread);

    void register_process(RefPtr<Process> process);

    static Thread* active_thread() { return s_current_thread; }

private:
    Scheduler() = default;

private:
    Thread* m_thread_queue;

    DynamicArray<RefPtr<Process>> m_processes;

    static Thread*    s_current_thread;
    static Scheduler* s_instance;
};
}
