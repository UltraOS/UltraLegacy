#include "Core/Runtime.h"

#include "Interrupts/IDT.h"
#include "Interrupts/Utilities.h"

#include "Scheduler.h"

namespace kernel {

InterruptSafeSpinLock Scheduler::s_queues_lock;

Scheduler* Scheduler::s_instance;

Scheduler::Scheduler()
    : m_expired_threads(new List<Thread>())
    , m_preemtable_threads(new List<Thread>())
{
}

void Scheduler::inititalize()
{
    s_instance = new Scheduler();

    Process::create_idle_for_this_processor();
    CPU::current().set_tss(*new TSS);
#ifdef ULTRA_64
    IDT::the().configure_ist();
#endif

    Timer::register_scheduler_handler(on_tick);
}

void Scheduler::sleep(u64 wake_time)
{
    ASSERT(!Interrupts::are_enabled());

    Thread::current()->sleep(wake_time);
    save_state_and_schedule();
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
        m_expired_threads->insert_back(*ready_thread);
    }
}

void Scheduler::exit(size_t)
{
    ASSERT(!"Scheduler::exit() is not yet implemented!");
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
        register_thread_unchecked(const_cast<Thread*>(thread.get()));
}

void Scheduler::register_thread(Thread* thread)
{
    LOCK_GUARD(s_queues_lock);

    ASSERT(m_processes.contains(thread->owner().id()));

    register_thread_unchecked(thread);
}

void Scheduler::register_thread_unchecked(Thread* thread)
{
    m_expired_threads->insert_back(*thread);
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
    if (m_preemtable_threads->empty()) {
        if (m_expired_threads->empty())
            return &CPU::current().idle_task();

        swap(m_preemtable_threads, m_expired_threads);
    }

    return &m_preemtable_threads->pop_front();
}

void Scheduler::pick_next()
{
    // Cannot use LOCK_GUARD here as switch_task never returns
    bool interrupt_state = false;
    s_queues_lock.lock(interrupt_state, __FILE__, __LINE__);

    auto* current_thread = Thread::current();

    wake_ready_threads();

    // if current thread is not dead/sleeping/blocked we put it back on the expired queue
    if (current_thread->is_running() && current_thread != &CPU::current().idle_task())
        m_expired_threads->insert_back(*current_thread);
    else if (current_thread->is_sleeping())
        m_sleeping_threads.emplace(current_thread);

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
