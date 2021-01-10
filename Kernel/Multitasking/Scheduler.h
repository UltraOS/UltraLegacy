#pragma once
#include "Common/Types.h"

#include "Common/Macros.h"
#include "Core/Registers.h"

#include "Process.h"
#include "Thread.h"

namespace kernel {

class Scheduler {
    MAKE_SINGLETON(Scheduler);

public:
    static void inititalize();
    static Scheduler& the();

    void yield();
    void sleep(u64 wake_time);
    [[noreturn]] void exit(size_t);
    [[noreturn]] void crash();

    void register_process(RefPtr<Process>);
    void register_thread(Thread*);

    struct Stats {
        DynamicArray<Pair<u32, StringView>> processor_to_task;
    };

    Stats stats() const;

private:
    // The 3 functions below assume s_queues_lock is held by the caller
    void register_thread_unchecked(Thread*); // Doesn't check if ID of owner is registered
    void wake_ready_threads();
    Thread* pick_next_thread();

    void pick_next();

    static void schedule(const RegisterState* registers);
    static void on_tick(const RegisterState&);
    static void switch_task(Thread::ControlBlock* new_task);
    static void save_state_and_schedule();

private:
    RedBlackTree<RefPtr<Process>, Less<>> m_processes; // sorted by pid

    RedBlackTree<Thread*, Thread::WakeTimePtrComparator> m_sleeping_threads; // sorted by wake-up time

    // A simplified version of the O(1) scheduler
    List<Thread>* m_expired_threads;
    List<Thread>* m_preemtable_threads;

    static InterruptSafeSpinLock s_queues_lock;

    static Scheduler* s_instance;
};
}
