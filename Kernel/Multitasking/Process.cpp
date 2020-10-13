#include "Interrupts/Utilities.h"

#include "Memory/MemoryManager.h"

#include "Scheduler.h"

#include "Process.h"

namespace kernel {

Atomic<u32>           Process::s_next_process_id;
InterruptSafeSpinLock Process::s_lock;

RefPtr<Process> Process::create(Address entrypoint, bool autoregister)
{
    LockGuard lock_guard(s_lock);

    RefPtr<Process> process = new Process(entrypoint);

    if (autoregister)
        Scheduler::the().register_process(process);

    return process;
}

// TODO: figure out the best way to fix the naming inconsistency here
//       e.g Process::create_supervious and Thread::create_supervisor_thread
RefPtr<Process> Process::create_supervisor(Address entrypoint)
{
    LockGuard lock_guard(s_lock);

    RefPtr<Process> process = new Process(entrypoint, true);
    Scheduler::the().register_process(process);

    return process;
}

void Process::inititalize_for_this_processor()
{
    LockGuard lock_guard(s_lock);

    // Create the initial idle proccess for this core
    RefPtr<Process> idle_process = new Process();

    idle_process->m_is_supervisor = true;

    // TODO: I don't like that we're turning a raw pointer into a RefPtr here
    //       Figure out a way to make it nicer.
    idle_process->m_address_space = &AddressSpace::of_kernel();

    auto idle_thread             = new Thread(AddressSpace::of_kernel(), nullptr);
    idle_thread->m_is_supervisor = true;
    idle_thread->activate();

    auto* last_picked = Scheduler::last_picked();

    if (!last_picked) {
        idle_thread->set_next(idle_thread);
        idle_thread->set_previous(idle_thread);
        Scheduler::set_last_picked(idle_thread);
    }

    Scheduler::the().register_process(idle_process);
    idle_process->m_threads.emplace(idle_thread);

    // enqueue if needed
    if (last_picked)
        Scheduler::enqueue_thread(*idle_thread);
}

void Process::commit()
{
    Scheduler::the().register_process(this);
}

Process::Process(Address entrypoint, bool is_supervisor)
    : m_process_id(s_next_process_id++), m_address_space(), m_is_supervisor(is_supervisor)
{
    if (is_user()) {
        auto& kernel_allocator = AddressSpace::of_kernel().allocator();
        auto  stack_range      = kernel_allocator.allocate_range(default_kernel_stack_size);
        MemoryManager::the().force_preallocate(stack_range, true);

        m_address_space      = RefPtr<AddressSpace>::create(MemoryManager::the().allocate_page());
        auto& user_allocator = m_address_space->allocator();

        auto main_thread = Thread::create_user_thread(*m_address_space,
                                                      user_allocator.allocate_range(default_userland_stack_size).end(),
                                                      stack_range.end(),
                                                      entrypoint);
        m_threads.emplace(main_thread);
    } else {
        auto& kernel_allocator = AddressSpace::of_kernel().allocator();
        auto  stack_range      = kernel_allocator.allocate_range(default_kernel_stack_size);
        MemoryManager::the().force_preallocate(stack_range, true);
        auto main_thread = Thread::create_supervisor_thread(stack_range.end(), entrypoint);

        m_threads.emplace(main_thread);
    }
}
}
