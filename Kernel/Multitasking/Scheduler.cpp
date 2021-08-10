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
    SleepBlocker blocker(*Thread::current(), wake_time);
    block(blocker);
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
        unblock_unchecked(*ready_thread);
    }
}

void Scheduler::exit_thread(i32 code)
{
    Interrupts::ScopedDisabler d;

    Thread::current()->exit(code);

    // We don't even have to save state because this thread will never be run again
    pick_next();
}

void Scheduler::exit_process(i32 code)
{
    Interrupts::ScopedDisabler d;

    auto& current_thread = *Thread::current();

    // We were the first to kill the process, no we get the right
    // to kill all other threads on exit if they exist.
    if (current_thread.owner().exit(code))
        current_thread.kill_all_threads_on_exit();

    current_thread.exit(code);

    pick_next();
}

// TODO: allow the user to somehow handle the error?
void Scheduler::crash(ErrorCode code)
{
    Interrupts::ScopedDisabler d;

    auto& current_thread = *Thread::current();

    // We were the first to kill the process, no we get the right
    // to kill all other threads on exit if they exist.
    if (current_thread.owner().exit(-code.value)) {
        current_thread.kill_all_threads_on_exit();
    }

    // Force set the exit code to our error.
    current_thread.exit(-code.value);
    current_thread.owner().set_exit_code(current_thread.exit_code());

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
    current_thread->set_state(Thread::State::DEAD);

    auto& process = current_thread->owner();

    // all we have to do is free the current thread
    if (!current_thread->should_kill_all_threads_on_exit()) {
        // For cases where last thread in the process calls exit_thread()
        if (process.decrement_alive_thread_count() == 0)
            process.exit(current_thread->exit_code());

        m_deferred_deleted_dead_threads.insert_back(*current_thread);
        return;
    }

    // We are the process killer, so we have to kill all threads.

    auto remove_sleeping_blocker = [this](SleepBlocker* blocker) {
        auto lb = m_sleeping_threads.lower_bound(blocker);

        while (lb != m_sleeping_threads.end() && *lb != blocker)
            ++lb;

        ASSERT(lb != m_sleeping_threads.end());
        ASSERT(*lb == blocker);

        m_sleeping_threads.remove(lb);
    };

    LOCK_GUARD(process.lock());
    for (const auto& thread : process.threads()) {
        if (thread.get() == current_thread) {
            process.decrement_alive_thread_count();
            m_deferred_deleted_dead_threads.insert_back(*current_thread);
            continue;
        }

        // tell the thread it's time for it to die
        thread->exit(0);

        if (thread->is_blocked()) {
            if (thread->blocker()->type() == Blocker::Type::SLEEP) {
                process.decrement_alive_thread_count();
                remove_sleeping_blocker(static_cast<SleepBlocker*>(thread->blocker()));
                TaskFinalizer::the().free_thread(*thread);
            } else if (thread->interrupt()) { // allow thread to clean-up if needed before exiting
                m_ready_threads.insert_back(*thread);
            }
        } else if (thread->is_ready()) {
            process.decrement_alive_thread_count();
            thread->pop_off();
            TaskFinalizer::the().free_thread(*thread);
        } // else thread is currently running, kill once it gets preempted OR thread is already dead
    }
}

void Scheduler::register_process(RefPtr<Process> process)
{
    LOCK_GUARD(s_queues_lock);

    ASSERT(!m_processes.contains(process->id()));

    m_processes.emplace(process);

    for (auto& thread : process->threads()) {
        thread->set_state(Thread::State::READY);
        process->increment_alive_thread_count();
        m_ready_threads.insert_back(*thread);
    }
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

bool Scheduler::register_thread(Thread& thread)
{
    LOCK_GUARD(s_queues_lock);

    // Tried to create a new thread for a dead process, kill it right away.
    auto& process = thread.owner();
    if (!process.is_alive()) {
        thread.exit(0);
        thread.set_state(Thread::State::DEAD);
        TaskFinalizer::the().free_thread(thread);
        return false;
    }

    ASSERT(m_processes.contains(thread.owner().id()));
    process.increment_alive_thread_count();
    thread.set_state(Thread::State::READY);
    m_ready_threads.insert_back(thread);
    return true;
}

void Scheduler::block(Blocker& blocker)
{
    Interrupts::ScopedDisabler d;

    Thread::current()->block(&blocker);

    save_state_and_schedule();
}

void Scheduler::unblock(Blocker& blocker)
{
    LOCK_GUARD(s_queues_lock);
    unblock_unchecked(blocker);
}

void Scheduler::unblock_unchecked(Blocker& blocker)
{
    auto& thread = blocker.thread();

    if (!thread.is_blocked()) {
        // Looks like we got unblocked too fast, the scheduler should
        // automatically unblock the thread once it gets into pick_next().
        blocker.set_result(Blocker::Result::UNBLOCKED);
        return;
    }

    thread.unblock();
    m_ready_threads.insert_back(thread);
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

    for (auto& cpu : CPU::processors())
        stats_per_cpu.processor_to_task.emplace(cpu.id(), cpu.current_thread()->owner().name().to_view());

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

    free_deferred_threads();
    wake_ready_threads();

    auto requested_state = current_thread->requested_state();

    if (current_thread->should_die() && !current_thread->is_invulnerable()) {
        kill_current_thread();
    } else if (requested_state != Thread::State::UNDEFINED) {
        switch (requested_state) {
        case Thread::State::BLOCKED:
            if (current_thread->blocker()->should_block()) {
                current_thread->set_state(Thread::State::BLOCKED);
                if (current_thread->blocker()->type() == Blocker::Type::SLEEP)
                    m_sleeping_threads.emplace(static_cast<SleepBlocker*>(current_thread->blocker()));
            } else { // got unblocked too early
                current_thread->unblock();
            }
            break;
        default:
            ASSERT_NEVER_REACHED();
        }

        current_thread->request_state(Thread::State::UNDEFINED, requested_state);
    }

    if (current_thread->is_running() && current_thread != &CPU::current().idle_task())
        m_ready_threads.insert_back(*current_thread);

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
