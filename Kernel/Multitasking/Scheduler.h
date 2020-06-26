#pragma once

#include "Common/Types.h"
#include "Interrupts/Common.h"
#include "Thread.h"

namespace kernel {

class Scheduler {
public:
    static void       inititalize();
    static Scheduler& the();

    static void on_tick(const RegisterState&);

    static void switch_task(Thread::ControlBlock* current_task, Thread::ControlBlock* new_task);

    static Thread* current_thread();

    void add_task(Thread& thread);

private:
    Scheduler() = default;

private:
    Thread*           m_threads;
    static Thread*    s_current_thread;
    static Scheduler* s_instance;
};
}
