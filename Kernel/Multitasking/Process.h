#pragma once

#include "Common/DynamicArray.h"
#include "Common/Set.h"

#include "FileSystem/FileIterator.h"
#include "Memory/VirtualRegion.h"
#include "TaskLoader.h"
#include "Thread.h"

namespace kernel {

class Process {
    MAKE_NONCOPYABLE(Process);
    MAKE_NONMOVABLE(Process);

public:
    enum class State : u32 {
        ALIVE,
        EXITING,
        EXITED
    };

    static constexpr u32 main_thread_id = 0;
    static constexpr auto default_userland_stack_size = 4 * MB;
    static constexpr auto default_kernel_stack_size = 32 * KB;

    static RefPtr<Process> create_idle_for_this_processor();

    static RefPtr<Process> create_supervisor(
        Address entrypoint,
        StringView name,
        size_t stack_size = default_kernel_stack_size);

    static RefPtr<Process> create_user(
        StringView name,
        AddressSpace* address_space,
        TaskLoader::LoadRequest*);

    ErrorCode create_thread(Address entrypoint, size_t stack_size = default_kernel_stack_size);

    Set<RefPtr<Thread>, Less<>>& threads() { return m_threads; }
    Set<RefPtr<VirtualRegion>, Less<>>& virtual_regions() { return m_virtual_regions; }
    Map<u32, RefPtr<IOStream>>& io_streams() { return m_io_streams; }
    RefPtr<IOStream> io_stream(u32 id);

    AddressSpace& address_space() { return *m_address_space; }

    [[nodiscard]] IsSupervisor is_supervisor() const { return m_is_supervisor; }

    [[nodiscard]] u32 consume_thread_id() { return m_next_thread_id++; }

    [[nodiscard]] u32 id() const { return m_id; }

    void set_exit_code(u32 code) { m_exit_code = code; }
    [[nodiscard]] u32 exit_code() const { return m_exit_code; }

    bool exit(u32 code)
    {
        State expected = State::ALIVE;
        bool did_set = m_state.compare_and_exchange(&expected, State::EXITING);

        if (did_set)
            m_exit_code = code;

        return did_set;
    }

    [[nodiscard]] bool is_alive() const { return m_state == State::ALIVE; }
    [[nodiscard]] State state() const { return m_state; }
    [[nodiscard]] const String& name() const { return m_name; }
    [[nodiscard]] InterruptSafeSpinLock& lock() const { return m_lock; }

    static Process& current() { return CPU::current().current_process(); }

    void set_working_directory(StringView);

    // NOTE: not thread safe (lock() must be held)
    const String& working_directory() { return m_working_directory; }

    void store_region(const RefPtr<VirtualRegion>&);
    ErrorCode store_io_stream_at(u32 id, const RefPtr<IOStream>&);
    ErrorOr<u32> store_io_stream(const RefPtr<IOStream>&);
    RefPtr<IOStream> pop_io_stream(u32 id);

    friend bool operator<(const RefPtr<Process>& l, const RefPtr<Process>& r)
    {
        return l->id() < r->id();
    }

    friend bool operator<(const RefPtr<Process>& l, u32 process_id)
    {
        return l->id() < process_id;
    }

    friend bool operator<(u32 process_id, const RefPtr<Process>& r)
    {
        return process_id < r->id();
    }

    ~Process()
    {
        ASSERT(m_state == State::EXITED);
    }

    [[nodiscard]] u32 alive_thread_count() const { return m_alive_thread_count; }
    u32 increment_alive_thread_count() { return ++m_alive_thread_count; }
    u32 decrement_alive_thread_count()
    {
        ASSERT(m_alive_thread_count != 0);
        return --m_alive_thread_count;
    }

private:
    Process(AddressSpace& address_space, IsSupervisor, StringView name);

    friend class TaskFinalizer;
    void mark_as_released()
    {
        ASSERT(m_state == State::EXITING);
        m_state = State::EXITED;
    }

private:
    u32 m_id { 0 };
    i32 m_exit_code { 0 };

    AddressSpace* m_address_space;
    Set<RefPtr<VirtualRegion>, Less<>> m_virtual_regions;
    Map<u32, RefPtr<IOStream>> m_io_streams;
    Set<RefPtr<Thread>, Less<>> m_threads;

    IsSupervisor m_is_supervisor { IsSupervisor::NO };

    String m_name;
    String m_working_directory { "/" };

    Atomic<u32> m_next_io_id { 10 };
    Atomic<u32> m_next_thread_id { main_thread_id };
    Atomic<u32> m_alive_thread_count { 0 }; // not always equal to m_threads.size()
    Atomic<State> m_state { State::ALIVE };

    mutable InterruptSafeSpinLock m_lock;

    static Atomic<u32> s_next_process_id;
};
}
