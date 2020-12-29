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
    static constexpr auto default_userland_stack_size = 4 * MB;
    static constexpr auto default_kernel_stack_size = 32 * KB;

    static void inititalize_for_this_processor();

    // TODO: add name?
    static RefPtr<Process> create(
        Address entrypoint,
        bool autoregister = true, // <--- this parameter is a temporary hack
        size_t stack_size = default_userland_stack_size);

    static RefPtr<Process> create_supervisor(Address entrypoint, size_t stack_size = default_kernel_stack_size);

    void commit();

    DynamicArray<RefPtr<Thread>>& threads() { return m_threads; }

    AddressSpace& address_space() { return *m_address_space; }

    [[nodiscard]] bool is_supervisor() const { return m_is_supervisor; }
    [[nodiscard]] bool is_user() const { return !is_supervisor(); }

private:
    Process(Address entrypoint, size_t stack_size, bool is_supervisor = false);
    Process() = default;

private:
    u32 m_process_id { 0 };
    RedBlackTree<RefPtr<VirtualRegion>, Less<>> m_virtual_regions;
    RefPtr<AddressSpace> m_address_space;
    bool m_is_supervisor { false };
    DynamicArray<RefPtr<Thread>> m_threads;

    static Atomic<u32> s_next_process_id;
    static InterruptSafeSpinLock s_lock;
};
}
