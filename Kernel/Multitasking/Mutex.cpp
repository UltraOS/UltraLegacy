#include "Mutex.h"
#include "Scheduler.h"

namespace kernel {

void Mutex::lock()
{
    for (;;) {
        bool interrupt_state = false;
        ILOCK(m_state_access_lock, interrupt_state);

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

void SharedMutex::exclusive_lock()
{
    for (;;) {
        bool interrupt_state = false;
        ILOCK(m_state_access_lock, interrupt_state);

        auto* current_thread = Thread::current();

        // Threads shouldn't be able to die when acquiring/owning a mutex
        ASSERT(current_thread->is_invulnerable());

        if (m_shared_refcount == free_mode) {
            m_shared_refcount = exclusive_mode;
            m_state_access_lock.unlock(interrupt_state);
            return;
        }

        MutexBlocker blocker(*current_thread);
        m_exclusive_waiters.insert_back(blocker);
        m_state_access_lock.unlock(interrupt_state);

        auto res = blocker.block();
        ASSERT(res == Blocker::Result::UNBLOCKED);
    }
}

void SharedMutex::exclusive_unlock()
{
    LOCK_GUARD(m_state_access_lock);

    ASSERT(m_shared_refcount == exclusive_mode);
    m_shared_refcount = free_mode;

    bool woke_one = false;

    while (!m_shared_waiters.empty()) {
        woke_one = true;
        auto& waiter = m_shared_waiters.pop_front();
        waiter.unblock();
    }

    if (woke_one)
        return;

    if (!m_exclusive_waiters.empty())
        m_exclusive_waiters.pop_front().unblock();
}

void SharedMutex::shared_lock()
{
    for (;;) {
        bool interrupt_state = false;
        ILOCK(m_state_access_lock, interrupt_state);

        auto* current_thread = Thread::current();

        // Threads shouldn't be able to die when acquiring/owning a mutex
        ASSERT(current_thread->is_invulnerable());

        if (m_shared_refcount >= free_mode) {
            m_shared_refcount++;
            m_state_access_lock.unlock(interrupt_state);
            return;
        }

        MutexBlocker blocker(*current_thread);
        m_shared_waiters.insert_back(blocker);
        m_state_access_lock.unlock(interrupt_state);

        auto res = blocker.block();
        ASSERT(res == Blocker::Result::UNBLOCKED);
    }
}

void SharedMutex::shared_unlock()
{
    LOCK_GUARD(m_state_access_lock);

    ASSERT(m_shared_refcount > free_mode);

    auto new_refcount = --m_shared_refcount;

    if (new_refcount == free_mode && !m_exclusive_waiters.empty())
        m_exclusive_waiters.pop_front().unblock();
}

}
