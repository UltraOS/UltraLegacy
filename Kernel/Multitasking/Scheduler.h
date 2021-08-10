#pragma once

#include "Common/Macros.h"
#include "Common/Set.h"
#include "Common/Types.h"

#include "Core/Registers.h"

#include "Process.h"
#include "Thread.h"

namespace kernel {

class Scheduler {
    MAKE_SINGLETON(Scheduler) = default;

public:
    static void inititalize();
    static bool is_initialized() { return s_instance != nullptr; }

    static Scheduler& the();

    void yield();
    void sleep(u64 wake_time);
    void block(Blocker&);
    void unblock(Blocker&);
    [[noreturn]] void exit_thread(i32);
    [[noreturn]] void exit_process(i32);
    [[noreturn]] void crash(ErrorCode);

    void register_process(RefPtr<Process>);
    RefPtr<Process> unregister_process(u32 id);

    bool register_thread(Thread&);

    struct Stats {
        DynamicArray<Pair<u32, StringView>> processor_to_task;
    };

    Stats stats() const;

private:
    // The 5 functions below assume s_queues_lock is held by the caller
    void wake_ready_threads();
    void unblock_unchecked(Blocker&);
    Thread* pick_next_thread();
    void kill_current_thread();
    void free_deferred_threads();

    [[noreturn]] void pick_next();

    static void schedule(const RegisterState* registers);
    static void on_tick(const RegisterState&);
    [[noreturn]] static void switch_task(Thread::ControlBlock* new_task);
    static void save_state_and_schedule();

private:
    Set<RefPtr<Process>, Less<>> m_processes; // sorted by pid

    MultiSet<SleepBlocker*, SleepBlocker::WakeTimePtrComparator> m_sleeping_threads; // sorted by wake-up time

    List<Thread> m_deferred_deleted_dead_threads;
    List<Thread> m_ready_threads;

    static InterruptSafeSpinLock s_queues_lock;

    static Scheduler* s_instance;
};
}
