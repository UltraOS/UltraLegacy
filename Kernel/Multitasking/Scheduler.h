#pragma once

#include "Common/Macros.h"
#include "Common/Set.h"
#include "Common/Types.h"

#include "Core/Registers.h"

#include "Process.h"
#include "Thread.h"

namespace kernel {

class Scheduler {
    MAKE_SINGLETON(Scheduler);

public:
    static void inititalize();
    static bool is_initialized() { return s_instance != nullptr; }

    static Scheduler& the();

    void yield();
    void sleep(u64 wake_time);
    [[noreturn]] void exit(size_t);
    [[noreturn]] void crash();

    void register_process(RefPtr<Process>);
    RefPtr<Process> unregister_process(u32 id);

    void register_thread(Thread*);

    struct Stats {
        DynamicArray<Pair<u32, StringView>> processor_to_task;
    };

    Stats stats() const;

private:
    // The 5 functions below assume s_queues_lock is held by the caller
    void register_thread_unchecked(Thread*); // Doesn't check if ID of owner is registered
    void wake_ready_threads();
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

    MultiSet<Thread*, Thread::WakeTimePtrComparator> m_sleeping_threads; // sorted by wake-up time

    List<Thread> m_deferred_deleted_dead_threads;

    // A simplified version of the O(1) scheduler
    List<Thread>* m_expired_threads;
    List<Thread>* m_preemtable_threads;

    static InterruptSafeSpinLock s_queues_lock;

    static Scheduler* s_instance;
};
}
