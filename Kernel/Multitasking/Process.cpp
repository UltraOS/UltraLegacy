#include "Process.h"
#include "FileSystem/VFS.h"
#include "Interrupts/Utilities.h"
#include "Memory/MemoryManager.h"
#include "Scheduler.h"

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

RefPtr<Process> Process::create_user(StringView name, AddressSpace* address_space, TaskLoader::LoadRequest* load_req)
{
    ASSERT(address_space != nullptr);
    RefPtr<Process> process = new Process(*address_space, IsSupervisor::NO, name);

    String process_name;
    process_name << name << " thread 0 stack"_sv;

    auto kernel_stack = MemoryManager::the().allocate_kernel_stack(process_name.to_view(), default_kernel_stack_size);

    auto main_thread = Thread::create_user(*process, kernel_stack, load_req);
    process->m_threads.emplace(main_thread);

    return process;
}

ErrorCode Process::create_thread(Address entrypoint, size_t stack_size)
{
    RefPtr<Thread> thread;

    if (!is_alive())
        return ErrorCode::ACCESS_DENIED;

    {
        LOCK_GUARD(m_lock);

        if (m_is_supervisor == IsSupervisor::YES) {
            String name = m_name;
            name << " thread " << m_next_thread_id.load(MemoryOrder::ACQUIRE) << " stack";
            auto stack = MemoryManager::the().allocate_kernel_stack(name.to_view(), stack_size);
            thread = Thread::create_supervisor(*this, stack, entrypoint);
        } else {
            ASSERT_NEVER_REACHED(); // trying to create user thread (TODO)
        }

        m_threads.emplace(thread);
    }

    // We got killed while creating this thread :(
    if (!Scheduler::the().register_thread(*thread))
        return ErrorCode::ACCESS_DENIED;

    return ErrorCode::NO_ERROR;
}

Process::Process(AddressSpace& address_space, IsSupervisor is_supervisor, StringView name)
    : m_id(s_next_process_id.fetch_add(1, MemoryOrder::ACQ_REL))
    , m_address_space(&address_space)
    , m_is_supervisor(is_supervisor)
    , m_name(name)
{
}

void Process::set_working_directory(StringView path)
{
    LOCK_GUARD(m_lock);
    m_working_directory = path;
}

void Process::store_region(const RefPtr<VirtualRegion>& region)
{
    LOCK_GUARD(m_lock);
    m_virtual_regions.emplace(region);
}

ErrorCode Process::store_io_stream_at(u32 id, const RefPtr<IOStream>& stream)
{
    LOCK_GUARD(m_lock);

    if (m_io_streams.contains(id))
        return ErrorCode::INVALID_ARGUMENT;

    m_io_streams.emplace(id, stream);

    // FIXME: support proper io id allocation.
    u32 next_io_id = max(m_next_io_id.load(MemoryOrder::ACQUIRE), id);
    m_next_io_id.store(next_io_id, MemoryOrder::RELEASE);

    return ErrorCode::NO_ERROR;
}

ErrorOr<u32> Process::store_io_stream(const RefPtr<IOStream>& object)
{
    LOCK_GUARD(m_lock);

    auto io_id = m_next_io_id.fetch_add(1, MemoryOrder::ACQ_REL);
    m_io_streams.emplace(io_id, object);

    return io_id;
}

RefPtr<IOStream> Process::pop_io_stream(u32 id)
{
    LOCK_GUARD(m_lock);

    auto io_stream = m_io_streams.find(id);

    if (io_stream == m_io_streams.end())
        return {};

    auto stream = io_stream->second;
    m_io_streams.remove(io_stream);

    return stream;
}

RefPtr<IOStream> Process::io_stream(u32 id)
{
    LOCK_GUARD(m_lock);

    auto io_stream = m_io_streams.find(id);

    return io_stream == m_io_streams.end() ? RefPtr<IOStream>() : io_stream->second;
}

}
