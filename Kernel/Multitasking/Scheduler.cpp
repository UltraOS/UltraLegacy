#include "Core/Runtime.h"

#include "Interrupts/Utilities.h"

#include "Scheduler.h"

namespace kernel {

RecursiveInterruptSafeSpinLock Scheduler::s_lock;

// 1 because the first process doesn't register itself
// TODO: fix that
size_t     Scheduler::s_thread_count = 1;
Thread*    Scheduler::s_last_picked;
Thread*    Scheduler::s_sleeping_threads;
Scheduler* Scheduler::s_instance;

void Scheduler::inititalize()
{
    Thread::initialize();
    s_instance = new Scheduler();
    Process::inititalize_for_this_processor();
}

void Scheduler::enqueue_thread(Thread& thread)
{
    bool interrupt_state = false;
    s_lock.lock(interrupt_state);

    ++s_thread_count;

    ASSERT(s_last_picked != nullptr);
    ASSERT(s_last_picked->has_next());
    ASSERT(s_last_picked->has_previous());

    auto* last_to_run = s_last_picked->previous();
    last_to_run->set_next(&thread);
    s_last_picked->set_previous(&thread);

    thread.set_next(s_last_picked);
    thread.set_previous(last_to_run);

    s_lock.unlock(interrupt_state);
}

void Scheduler::dequeue_thread(Thread& thread)
{
    bool interrupt_state = false;
    s_lock.lock(interrupt_state);

    --s_thread_count;

    ASSERT(thread.has_previous());
    ASSERT(thread.has_next());

    thread.previous()->set_next(thread.next());
    thread.next()->set_previous(thread.previous());

    s_lock.unlock(interrupt_state);
}

void Scheduler::enqueue_sleeping_thread(Thread& thread)
{
    bool interrupt_state = false;
    s_lock.lock(interrupt_state);

    ASSERT(&thread != s_sleeping_threads);

    if (!s_sleeping_threads) {
        s_sleeping_threads = &thread;
        thread.set_next(nullptr);
    } else {
        thread.set_next(s_sleeping_threads);
        s_sleeping_threads = &thread;
    }

    s_lock.unlock(interrupt_state);
}

void Scheduler::register_process(RefPtr<Process> process)
{
    bool interrupt_state = false;
    s_lock.lock(interrupt_state);

    m_processes.emplace(process);

    for (auto& thread: process->threads())
        enqueue_thread(*thread);

    s_lock.unlock(interrupt_state);
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

// Yes, the algorithm here is terrible
// TODO: replace it with something more "appropriate"
void Scheduler::pick_next()
{
    bool interrupt_state = false;
    s_lock.lock(interrupt_state);

    wake_up_ready_threads();

    auto* current_thread = Thread::current();

    auto* next_thread = s_last_picked->next();

    for (size_t i = 0; i < s_thread_count; ++i) {
        if (next_thread->is_ready())
            break;

        next_thread = next_thread->next();
    }

    if (current_thread->is_sleeping()) {
        ASSERT(current_thread != next_thread);
        ASSERT(!next_thread->is_running());

        dequeue_thread(*current_thread);

        enqueue_sleeping_thread(*current_thread);

    } else if (current_thread->is_dead()) {
        ASSERT(current_thread != next_thread);
        ASSERT(!next_thread->is_running());

        dequeue_thread(*current_thread);

    } else if (current_thread == next_thread || next_thread->is_running()) {
        s_lock.unlock(interrupt_state);
        return;
    }

    current_thread->deactivate();
    next_thread->activate();
    s_last_picked = next_thread;

    s_lock.unlock(interrupt_state);

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
