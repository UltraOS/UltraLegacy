#pragma once

#include "Common/DynamicArray.h"
#include "Common/RedBlackTree.h"
#include "Memory/VirtualRegion.h"
#include "Thread.h"

namespace kernel {

class Process {
    MAKE_NONCOPYABLE(Process);
    MAKE_NONMOVABLE(Process);

public:
    static constexpr u32 main_thread_id = 0;
    static constexpr auto default_userland_stack_size = 4 * MB;
    static constexpr auto default_kernel_stack_size = 32 * KB;

    static RefPtr<Process> create_idle_for_this_processor();

    static RefPtr<Process> create_supervisor(
        Address entrypoint,
        StringView name,
        size_t stack_size = default_kernel_stack_size);

    DynamicArray<RefPtr<Thread>>& threads() { return m_threads; }

    AddressSpace& address_space() { return m_address_space; }

    [[nodiscard]] IsSupervisor is_supervisor() const { return m_is_supervisor; }

    [[nodiscard]] u32 consume_thread_id() { return m_next_thread_id++; }

    [[nodiscard]] u32 id() const { return m_id; }

    [[nodiscard]] const String& name() const { return m_name; }

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

private:
    Process(AddressSpace& address_space, IsSupervisor, StringView name);

private:
    u32 m_id { 0 };

    AddressSpace& m_address_space;
    RedBlackTree<RefPtr<VirtualRegion>, Less<>> m_virtual_regions;

    IsSupervisor m_is_supervisor { IsSupervisor::NO };
    DynamicArray<RefPtr<Thread>> m_threads;

    String m_name;

    Atomic<u32> m_next_thread_id { main_thread_id + 1 };

    static Atomic<u32> s_next_process_id;
    static InterruptSafeSpinLock s_lock;
};
}
