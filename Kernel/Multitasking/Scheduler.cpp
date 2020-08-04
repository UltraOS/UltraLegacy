#include "Core/Runtime.h"

#include "Interrupts/Utilities.h"

#include "Scheduler.h"

namespace kernel {

Scheduler* Scheduler::s_instance;
Thread*    Scheduler::s_sleeping_threads;

void Scheduler::inititalize()
{
    Thread::initialize();
    s_instance = new Scheduler();
    Process::inititalize();
}

void Scheduler::enqueue_thread(Thread& thread)
{
    ASSERT(Thread::current() != nullptr);
    ASSERT(Thread::current()->has_next());
    ASSERT(Thread::current()->has_previous());

    auto* last_to_run = Thread::current()->previous();
    last_to_run->set_next(&thread);
    Thread::current()->set_previous(&thread);

    thread.set_next(Thread::current());
    thread.set_previous(last_to_run);
}

void Scheduler::dequeue_thread(Thread& thread)
{
    ASSERT(thread.has_previous());
    ASSERT(thread.has_next());

    thread.previous()->set_next(thread.next());
    thread.next()->set_previous(thread.previous());
}

void Scheduler::enqueue_sleeping_thread(Thread& thread)
{
    ASSERT(&thread != s_sleeping_threads);

    if (!s_sleeping_threads) {
        s_sleeping_threads = &thread;
        thread.set_next(nullptr);
    } else {
        thread.set_next(s_sleeping_threads);
        s_sleeping_threads = &thread;
    }
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
    Interrupts::ScopedDisabler d;
    save_state_and_schedule();
}

void Scheduler::wake_up_ready_threads()
{
    auto*          next    = s_sleeping_threads;
    decltype(next) current = nullptr;
    s_sleeping_threads     = nullptr;

    while (next) {
        current = next;
        next    = current->next();
        ASSERT(current != next);

        if (current->should_be_woken_up()) {
            current->wake_up();
            enqueue_thread(*current);
        } else
            enqueue_sleeping_thread(*current);
    }
}

void Scheduler::pick_next()
{
    wake_up_ready_threads();

    auto* current_thread = Thread::current();
    auto* next_thread    = current_thread->next();

    ASSERT(current_thread->has_previous());
    ASSERT(current_thread->has_next());

    ASSERT(next_thread->has_previous());
    ASSERT(next_thread->has_next());

    if (current_thread->is_sleeping()) {
        ASSERT(current_thread != next_thread);

        dequeue_thread(*current_thread);

        enqueue_sleeping_thread(*current_thread);

    } else if (current_thread->is_dead()) {
        ASSERT(current_thread != next_thread);

        dequeue_thread(*current_thread);

    } else if (current_thread == next_thread)
        return;

    current_thread->deactivate();
    next_thread->activate();

    switch_task(next_thread->control_block());
}

void Scheduler::schedule(const RegisterState* registers)
{
    Thread::current()->control_block()->current_kernel_stack_top = registers;
    pick_next();
}

void Scheduler::on_tick(const RegisterState& registers)
{
    schedule(&registers);
}
}
