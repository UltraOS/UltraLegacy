#pragma once

#include "Common/RefPtr.h"
#include "Process.h"
#include "Thread.h"

namespace kernel {

class TaskFinalizer {
    MAKE_SINGLETON(TaskFinalizer);

public:
    static void spawn();

    static TaskFinalizer& the()
    {
        ASSERT(s_instance != nullptr);
        return *s_instance;
    }

    void free_thread(Thread&);

private:
    [[noreturn]] static void run();
    bool finalize_task_queue();
    void swap_queues();
    void do_free_thread(Thread& thread);
    void do_free_process(RefPtr<Process>&& process);

private:
    InterruptSafeSpinLock m_threads_to_free_list_lock;
    List<Thread>* m_threads_to_free;

    List<Thread>* m_threads_being_freed;
    List<RefPtr<Process>> m_processes_to_free;

    static TaskFinalizer* s_instance;
};
}
