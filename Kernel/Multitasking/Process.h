#pragma once
#include "Common/DynamicArray.h"
#include "Thread.h"

namespace kernel {

class Process {
public:
    static constexpr auto default_userland_stack_size = 4 * MB;
    static constexpr auto default_kerenl_stack_size   = 1 * MB;

    static void inititalize();

    // TODO: add name?
    static RefPtr<Process> create(Address entrypoint);
    static RefPtr<Process> create_supervisor(Address entrypoint);

    DynamicArray<RefPtr<Thread>>& threads() { return m_threads; }

    static Process& kernel()
    {
        ASSERT(s_kernel_process.get() != nullptr);

        return *s_kernel_process;
    }

    bool is_supervisor() const { return m_is_supervisor; }
    bool is_user() const { return !is_supervisor(); }

private:
    Process(Address entrypoint, bool is_supervisor = false);
    Process() = default;

private:
    u32                          m_process_id { 0 };
    RefPtr<PageDirectory>        m_page_directory;
    bool                         m_is_supervisor { false };
    DynamicArray<RefPtr<Thread>> m_threads;

    static u32             s_next_process_id;
    static RefPtr<Process> s_kernel_process;
};
}
