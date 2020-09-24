#pragma once
#include "Common/DynamicArray.h"
#include "Thread.h"

namespace kernel {

class Process {
public:
    static constexpr auto default_userland_stack_size = 4 * MB;
    static constexpr auto default_kernel_stack_size   = 16 * KB;

    static void inititalize_for_this_processor();

    // TODO: add name?                                     vvv this parameter is a temporary hack
    static RefPtr<Process> create(Address entrypoint, bool autoregister = true);
    static RefPtr<Process> create_supervisor(Address entrypoint);

    void commit();

    DynamicArray<RefPtr<Thread>>& threads() { return m_threads; }

    AddressSpace& address_space() { return *m_address_space; }

    bool is_supervisor() const { return m_is_supervisor; }
    bool is_user() const { return !is_supervisor(); }

private:
    Process(Address entrypoint, bool is_supervisor = false);
    Process() = default;

private:
    u32                          m_process_id { 0 };
    RefPtr<AddressSpace>         m_address_space;
    bool                         m_is_supervisor { false };
    DynamicArray<RefPtr<Thread>> m_threads;

    static Atomic<u32>           s_next_process_id;
    static InterruptSafeSpinLock s_lock;
};
}
