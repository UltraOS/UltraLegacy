#include "Process.h"
#include "Memory/MemoryManager.h"
#include "Scheduler.h"

namespace kernel {

u32             Process::s_next_process_id;
RefPtr<Process> Process::s_kernel_process;

RefPtr<Process> Process::create(Address entrypoint)
{
    InterruptDisabler d;

    RefPtr<Process> process = new Process(entrypoint);
    Scheduler::the().register_process(process);
    return process;
}

// TODO: figure out the best way to fix the naming inconsistency here
//       e.g Process::create_supervious and Thread::create_supervisor_thread
RefPtr<Process> Process::create_supervisor(Address entrypoint)
{
    InterruptDisabler d;

    RefPtr<Process> process = new Process(entrypoint, true);
    Scheduler::the().register_process(process);
    return process;
}

// defined in Core/crt0.asm
extern "C" Address kernel_stack_begin;

void Process::inititalize()
{
    s_kernel_process                  = new Process();
    s_kernel_process->m_is_supervisor = true;

    // TODO: I don't like that we're turning a raw pointer into a RefPtr here
    //       Figure out a way to make it nicer.
    s_kernel_process->m_page_directory = &PageDirectory::of_kernel();

    auto main_kernel_thread             = new Thread(PageDirectory::of_kernel(), &kernel_stack_begin);
    main_kernel_thread->m_is_supervisor = true;
    main_kernel_thread->activate();
    main_kernel_thread->set_next(main_kernel_thread);
    main_kernel_thread->set_previous(main_kernel_thread);

    Scheduler::the().register_process(s_kernel_process);

    s_kernel_process->m_threads.emplace(main_kernel_thread);
}

Process::Process(Address entrypoint, bool is_supervisor)
    : m_process_id(s_next_process_id++), m_page_directory(), m_is_supervisor(is_supervisor)
{
    if (is_user()) {
        m_page_directory       = RefPtr<PageDirectory>::create(MemoryManager::the().allocate_page());
        auto& user_allocator   = m_page_directory->allocator();
        auto& kernel_allocator = PageDirectory::of_kernel().allocator();
        auto  main_thread      = Thread::create_user_thread(*m_page_directory,
                                                      user_allocator.allocate_range(default_userland_stack_size).end(),
                                                      kernel_allocator.allocate_range(default_kerenl_stack_size).end(),
                                                      entrypoint);
        m_threads.emplace(main_thread);
    } else {
        auto& kernel_allocator = PageDirectory::of_kernel().allocator();
        auto  main_thread
            = Thread::create_supervisor_thread(kernel_allocator.allocate_range(default_kerenl_stack_size).end(),
                                               entrypoint);

        m_threads.emplace(main_thread);
    }
}
}
