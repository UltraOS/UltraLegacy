#include "Core/Runtime.h"

#include "Scheduler.h"

namespace kernel {

Scheduler* Scheduler::s_instance;
Thread*    Scheduler::s_first_ready_to_run_thread;
Thread*    Scheduler::s_last_ready_to_run_thread;

void Scheduler::inititalize()
{
    Thread::initialize();
    s_instance = new Scheduler();
    Process::inititalize();
}

void Scheduler::enqueue_thread(Thread& thread)
{
    if (!s_first_ready_to_run_thread && !s_last_ready_to_run_thread) {
        s_first_ready_to_run_thread = &thread;
        s_last_ready_to_run_thread  = &thread;
        thread.activate();
        return;
    }

    if (!s_first_ready_to_run_thread)
        s_first_ready_to_run_thread = &thread;

    s_last_ready_to_run_thread->set_next(&thread);
    s_last_ready_to_run_thread = &thread;
}

void Scheduler::register_process(RefPtr<Process> process)
{
    m_processes.emplace(process);

    for (auto& thread: process->threads())
        enqueue_thread(*thread);
}

Scheduler& Scheduler::the()
{
    ASSERT(s_instance != nullptr);

    return *s_instance;
}

void Scheduler::yield()
{
    InterruptDisabler d;
    pick_next();
}

void Scheduler::pick_next()
{
    if (!s_first_ready_to_run_thread) {
        log() << "Scheduler: no threads to switch to, skipping...";
        return;
    }

    auto* current_thread = Thread::current();
    current_thread->deactivate();

    auto* next_thread           = s_first_ready_to_run_thread;
    s_first_ready_to_run_thread = next_thread->next();
    enqueue_thread(*current_thread);

    next_thread->activate();

    switch_task(current_thread->control_block(), next_thread->control_block());
}

void Scheduler::on_tick(const RegisterState&)
{
    pick_next();
}
}
