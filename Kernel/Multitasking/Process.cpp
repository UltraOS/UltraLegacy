#include "Interrupts/Utilities.h"

#include "Memory/MemoryManager.h"

#include "Scheduler.h"

#include "Process.h"

namespace kernel {

Atomic<u32> Process::s_next_process_id;
InterruptSafeSpinLock Process::s_lock;

RefPtr<Process> Process::create(Address entrypoint, bool autoregister, size_t stack_size)
{
    LOCK_GUARD(s_lock);

    RefPtr<Process> process = new Process(entrypoint, stack_size);

    if (autoregister)
        Scheduler::the().register_process(process);

    return process;
}

// TODO: figure out the best way to fix the naming inconsistency here
//       e.g Process::create_supervious and Thread::create_supervisor_thread
RefPtr<Process> Process::create_supervisor(Address entrypoint, size_t stack_size)
{
    LOCK_GUARD(s_lock);

    RefPtr<Process> process = new Process(entrypoint, stack_size, true);
    Scheduler::the().register_process(process);

    return process;
}

void Process::inititalize_for_this_processor()
{
    LOCK_GUARD(s_lock);

    // Create the initial idle proccess for this core
    RefPtr<Process> idle_process = new Process();

    idle_process->m_is_supervisor = true;

    // TODO: I don't like that we're turning a raw pointer into a RefPtr here
    //       Figure out a way to make it nicer.
    idle_process->m_address_space = &AddressSpace::of_kernel();

    auto idle_thread = new Thread(AddressSpace::of_kernel(), nullptr);
    idle_thread->m_is_supervisor = IsSupervisor::YES;
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

Process::Process(Address entrypoint, size_t stack_size, bool is_supervisor)
    : m_process_id(s_next_process_id++)
    , m_address_space()
    , m_is_supervisor(is_supervisor)
{
    String kstack_message;
    kstack_message << "PID " << m_process_id << " kernel stack";

    if (is_user()) {
        String userstack_message;
        userstack_message << "PID " << m_process_id << " user stack";
        auto user_stack = MemoryManager::the().allocate_user_stack(userstack_message.to_view(), *m_address_space, stack_size);
        auto kernel_stack = MemoryManager::the().allocate_kernel_stack(kstack_message.to_view());

        m_address_space = RefPtr<AddressSpace>::create();

        auto main_thread = Thread::create_user_thread(
            *m_address_space,
            user_stack->virtual_range().end(),
            kernel_stack->virtual_range().end(),
            entrypoint);
        m_threads.emplace(main_thread);
    } else {
        auto kernel_stack = MemoryManager::the().allocate_kernel_stack(kstack_message.to_view(), stack_size);
        auto main_thread = Thread::create_supervisor_thread(kernel_stack->virtual_range().end(), entrypoint);
        m_threads.emplace(main_thread);
    }
}
}
