#include "Core/Runtime.h"

#include "Interrupts/IDT.h"
#include "Interrupts/Utilities.h"

#include "Scheduler.h"
#include "TaskFinalizer.h"

namespace kernel {

InterruptSafeSpinLock Scheduler::s_queues_lock;

Scheduler* Scheduler::s_instance;

void Scheduler::inititalize()
{
    s_instance = new Scheduler();

    Process::create_idle_for_this_processor();
    CPU::current().set_tss(*new TSS);
#ifdef ULTRA_64
    IDT::the().configure_ist();
#endif

    TaskFinalizer::spawn();

    Timer::register_scheduler_handler(on_tick);
}

void Scheduler::sleep(u64 wake_time)
{
    Interrupts::ScopedDisabler d;

    auto* current_thread = Thread::current();
    bool success = false;

    {
        LOCK_GUARD(s_queues_lock);

        success = current_thread->sleep(wake_time);

        if (success)
            m_sleeping_threads.emplace(current_thread);
    }

    if (success)
        save_state_and_schedule();
    else
        // If sleep() fails we assume the thread's state
        // is set to dead, therefore we can afford to call
        // pick_next() and not save the state of the thread.
        pick_next();
}

void Scheduler::wake_ready_threads()
{
    if (m_sleeping_threads.empty())
        return;

    for (auto thread_itr = m_sleeping_threads.begin(); thread_itr != m_sleeping_threads.end();) {
        if (!(*thread_itr)->should_be_woken_up())
            return; // we can afford to do this since threads are sorted in wake-up time order

        auto* ready_thread = *thread_itr;
        m_sleeping_threads.remove(thread_itr++);
        ready_thread->wake_up();
        m_ready_threads.insert_back(*ready_thread);
    }
}

void Scheduler::exit(size_t)
{
    Interrupts::ScopedDisabler d;

    Thread::current()->exit();

    // We don't even have to save state because this thread will never be ran again
    pick_next();
}

void Scheduler::free_deferred_threads()
{
    for (auto itr = m_deferred_deleted_dead_threads.begin(); itr != m_deferred_deleted_dead_threads.end(); ++itr) {
        auto* thread = itr.node();
        m_deferred_deleted_dead_threads.pop(itr++);
        TaskFinalizer::the().free_thread(*thread);
    }
}

void Scheduler::kill_current_thread()
{
    auto* current_thread = Thread::current();

    if (!current_thread->is_main()) {
        ASSERT(current_thread->is_dead());
        // We have to defer freeing of this thread, otherwise
        // it becomes a race condition between scheduler and task finalizer
        m_deferred_deleted_dead_threads.insert_back(*current_thread);
        return;
    }

    auto find_sleeping_thread = [this](Thread* thread) -> decltype(m_sleeping_threads)::ConstIterator {
        auto lb = m_sleeping_threads.lower_bound(thread);

        while (lb != m_sleeping_threads.end() && *lb != thread)
            ++lb;

        ASSERT(lb != m_sleeping_threads.end());
        ASSERT(*lb == thread);

        return lb;
    };

    // Killing the main thread so we also have to kill every other thread
    LOCK_GUARD(current_thread->owner().lock());
    for (const auto& thread : current_thread->owner().threads()) {
        auto previous_state = thread->exit();

        // Thread we just tried to kill was blocked with a non-cancellable blocker,
        // so we don't try to free it ourselves. Instead it will be done once the thread
        // is unblocked.
        if (previous_state == Thread::State::DELAYED_DEAD)
            continue;

        if (thread.get() == current_thread) {
            m_deferred_deleted_dead_threads.insert_back(*current_thread);
            continue;
        }

        // since this thread is not running we can afford to free it right away
        if (previous_state == Thread::State::SLEEPING) {
            auto this_thread = find_sleeping_thread(thread.get());
            m_sleeping_threads.remove(this_thread);
            TaskFinalizer::the().free_thread(*thread);
        } else if (previous_state == Thread::State::BLOCKED) { // thread was blocked by a cancellable blocker
            TaskFinalizer::the().free_thread(*thread);
        } else if (thread->is_on_a_list()) {
            thread->pop_off();
            TaskFinalizer::the().free_thread(*thread);
        } // else thread is currently running, kill once it gets preempted OR thread is already dead
    }
}

void Scheduler::crash()
{
    ASSERT(!"Scheduler::crash() is not yet implemented!");
}

void Scheduler::register_process(RefPtr<Process> process)
{
    LOCK_GUARD(s_queues_lock);

    ASSERT(!m_processes.contains(process->id()));

    m_processes.emplace(process);

    for (auto& thread : process->threads())
        register_thread_unchecked(thread.get());
}

RefPtr<Process> Scheduler::unregister_process(u32 id)
{
    LOCK_GUARD(s_queues_lock);

    auto this_process = m_processes.find(id);
    ASSERT(this_process != m_processes.end());

    auto ref = *this_process;
    m_processes.remove(id);

    return ref;
}

void Scheduler::register_thread(Thread* thread)
{
    LOCK_GUARD(s_queues_lock);

    ASSERT(m_processes.contains(thread->owner().id()));

    register_thread_unchecked(thread);
}

void Scheduler::requeue_unblocked_thread(Thread* thread)
{
    LOCK_GUARD(s_queues_lock);

    if (thread->is_on_a_list()) {
        ASSERT(thread->list() == &m_blocked_threads);
        thread->pop_off();
        thread->set_state(Thread::State::READY);
        register_thread_unchecked(thread);
    } else { // thread was already running
        thread->set_state(Thread::State::RUNNING);
    }
}

void Scheduler::register_thread_unchecked(Thread* thread)
{
    m_ready_threads.insert_back(*thread);
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

Scheduler::Stats Scheduler::stats() const
{
    Stats stats_per_cpu;

    LOCK_GUARD(s_queues_lock);

    for (auto& id_to_cpu : CPU::processors())
        stats_per_cpu.processor_to_task.emplace(id_to_cpu.first(), id_to_cpu.second().current_thread()->owner().name().to_view());

    return stats_per_cpu;
}

Thread* Scheduler::pick_next_thread()
{
    if (m_ready_threads.empty())
        return &CPU::current().idle_task();

    return &m_ready_threads.pop_front();
}

void Scheduler::pick_next()
{
    // Cannot use LOCK_GUARD here as switch_task never returns
    bool interrupt_state = false;
    s_queues_lock.lock(interrupt_state, __FILE__, __LINE__);

    auto* current_thread = Thread::current();

    bool interrupt_state_unused = false;
    current_thread->state_lock().lock(interrupt_state_unused);

    free_deferred_threads();
    wake_ready_threads();

    // if current thread is not dead/sleeping/blocked we put it back on the expired queue
    if ((current_thread->is_running() || current_thread->is_ready()) && current_thread != &CPU::current().idle_task()) {
        m_ready_threads.insert_back(*current_thread);
    } else if (current_thread->is_blocked()) {
        m_blocked_threads.insert_back(*current_thread);
    } else if (current_thread->is_dead()) {
        current_thread->state_lock().unlock(interrupt_state_unused);
        kill_current_thread();
    }

    current_thread->state_lock().unlock(interrupt_state_unused);
    auto* next_thread = pick_next_thread();

    if (current_thread != next_thread) {
        current_thread->deactivate();
        next_thread->activate();
    }

    s_queues_lock.unlock(interrupt_state);

    switch_task(next_thread->control_block());
}

void Scheduler::schedule(const RegisterState* ptr)
{
    Thread::current()->control_block()->current_kernel_stack_top = reinterpret_cast<ptr_t>(ptr);
    s_instance->pick_next();
}

void Scheduler::on_tick(const RegisterState& registers)
{
    schedule(&registers);
}
}
