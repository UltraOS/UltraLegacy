#include "Core/Runtime.h"

#include "Scheduler.h"

namespace kernel {

Scheduler* Scheduler::s_instance;
Thread*    Scheduler::s_current_thread;

void Scheduler::inititalize()
{
    Thread::initialize();
    s_instance = new Scheduler();
    Process::inititalize();
}

void Scheduler::enqueue_thread(Thread& thread)
{
    if (!m_thread_queue) {
        m_thread_queue   = &thread;
        s_current_thread = &thread;
        return;
    }

    auto* last = s_current_thread->next();
    s_current_thread->set_next(&thread);

    if (last)
        thread.set_next(last);
    else
        thread.set_next(s_current_thread);
}

void Scheduler::register_process(RefPtr<Process> process)
{
    m_processes.emplace(process);

    for (auto& thread: process->threads())
        enqueue_thread(*thread);
}

Thread& Scheduler::current_thread()
{
    ASSERT(s_current_thread);

    return *s_current_thread;
}

Scheduler& Scheduler::the()
{
    ASSERT(s_instance != nullptr);

    return *s_instance;
}

void Scheduler::on_tick(const RegisterState&)
{
    if (!s_current_thread->has_next()) {
        log() << "Scheduler: no threads to switch to, skipping...";
        return;
    }

    auto* old_thread = s_current_thread;
    s_current_thread = s_current_thread->next();

    old_thread->deactivate();
    s_current_thread->activate();

    switch_task(old_thread->control_block(), s_current_thread->control_block());
}
}
