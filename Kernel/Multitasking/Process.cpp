#include "Interrupts/Utilities.h"
#include "Memory/MemoryManager.h"
#include "Scheduler.h"
#include "FileSystem/VFS.h"
#include "Process.h"

namespace kernel {

Atomic<u32> Process::s_next_process_id;

RefPtr<Process> Process::create_idle_for_this_processor()
{
    String process_name = "cpu ";
    process_name << CPU::current_id() << " idle task";

    RefPtr<Process> process = new Process(AddressSpace::of_kernel(), IsSupervisor::YES, process_name.to_view());

    auto main_thread = Thread::create_idle(*process);
    process->m_threads.emplace(main_thread);

    CPU::current().set_idle_task(process);

    return process;
}

RefPtr<Process> Process::create_supervisor(Address entrypoint, StringView name, size_t stack_size)
{
    RefPtr<Process> process = new Process(AddressSpace::of_kernel(), IsSupervisor::YES, name);

    String process_name;
    process_name << name << " thread 0 stack"_sv;

    auto stack = MemoryManager::the().allocate_kernel_stack(process_name.to_view(), stack_size);

    auto main_thread = Thread::create_supervisor(*process, stack, entrypoint);
    process->m_threads.emplace(main_thread);

    Scheduler::the().register_process(process);

    return process;
}

RefPtr<Process> Process::create_user(StringView name, AddressSpace* address_space, TaskLoader::LoadRequest* load_req, size_t stack_size)
{
    ASSERT(address_space != nullptr);
    RefPtr<Process> process = new Process(*address_space, IsSupervisor::NO, name);

    String process_name;
    process_name << name << " thread 0 stack"_sv;

    auto kernel_stack = MemoryManager::the().allocate_kernel_stack(process_name.to_view(), default_kernel_stack_size);
    auto user_stack = MemoryManager::the().allocate_user_stack(process_name.to_view(), *address_space, stack_size);

    auto main_thread = Thread::create_user(*process, kernel_stack, user_stack, load_req);
    process->m_threads.emplace(main_thread);

    return process;
}

void Process::create_thread(Address entrypoint, size_t stack_size)
{
    RefPtr<Thread> thread;

    {
        LOCK_GUARD(m_lock);

        // Trying to create a thread for a dead process
        // TODO: add some sort of handling for this case
        ASSERT(!m_threads.empty() && !m_threads.begin()->get()->is_dead());

        if (m_is_supervisor == IsSupervisor::YES) {
            String name = m_name;
            name << " thread " << m_next_thread_id.load() << " stack";
            auto stack = MemoryManager::the().allocate_kernel_stack(name.to_view(), stack_size);
            thread = Thread::create_supervisor(*this, stack, entrypoint);
        } else {
            ASSERT_NEVER_REACHED(); // trying to create user thread (TODO)
        }

        m_threads.emplace(thread);
    }

    Scheduler::the().register_thread(*thread);
}

Process::Process(AddressSpace& address_space, IsSupervisor is_supervisor, StringView name)
    : m_id(s_next_process_id++)
    , m_address_space(&address_space)
    , m_is_supervisor(is_supervisor)
    , m_name(name)
{
}

void Process::store_region(const RefPtr<VirtualRegion>& region)
{
    LOCK_GUARD(m_lock);
    m_virtual_regions.emplace(region);
}

u32 Process::store_fd(const RefPtr<FileDescription>& fd)
{
    LOCK_GUARD(m_lock);

    auto fd_id = m_next_fd_id++;
    m_fds.emplace(fd_id, fd);

    return fd_id;
}

ErrorCode Process::close_fd(u32 id)
{
    LOCK_GUARD(m_lock);

    auto fd = m_fds.find(id);

    if (fd == m_fds.end())
        return ErrorCode::INVALID_ARGUMENT;

    VFS::the().close(*fd->second);
    m_fds.remove(id);

    return ErrorCode::NO_ERROR;
}

RefPtr<FileDescription> Process::fd(u32 id)
{
    LOCK_GUARD(m_lock);

    auto fd = m_fds.find(id);

    return fd == m_fds.end() ? RefPtr<FileDescription>() : fd->second;
}

}
