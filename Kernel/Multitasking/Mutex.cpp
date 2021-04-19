#include "Mutex.h"
#include "Scheduler.h"

namespace kernel {

void Mutex::lock()
{
   for (;;) {
       bool interrupt_state = false;
       m_state_access_lock.lock(interrupt_state, __FILE__, __LINE__, CPU::current_id());

       auto* current_thread = Thread::current();

       // Threads shouldn't be able to die when acquiring/owning a mutex
       ASSERT(current_thread->is_invulnerable());

       if (m_owner == current_thread)
           FAILED_ASSERTION("Mutex deadlock");

       if (m_owner == nullptr) {
           m_owner = current_thread;
           m_state_access_lock.unlock(interrupt_state);
           return;
       }

       MutexBlocker blocker(*current_thread);
       m_waiters.insert_back(blocker);
       m_state_access_lock.unlock(interrupt_state);

       auto res = blocker.block();
       ASSERT(res == Blocker::Result::UNBLOCKED);
   }
}

void Mutex::unlock()
{
    LOCK_GUARD(m_state_access_lock);

    ASSERT(m_owner == Thread::current());

    m_owner = nullptr;

    if (m_waiters.empty())
        return;

    auto& blocker = m_waiters.pop_front();
    blocker.unblock();
}


}