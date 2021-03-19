#include "Storage.h"

namespace kernel {

StorageDevice::AsyncRequest::AsyncRequest(Address virtual_address, LBARange lba_range, AsyncRequest::Type type)
    : m_blocker(*Thread::current())
    , m_virtual_address(virtual_address)
    , m_lba_range(lba_range)
    , m_type(type)
{
}

bool StorageDevice::AsyncRequest::begin()
{
    return m_blocker.block();
}

void StorageDevice::AsyncRequest::wait()
{
    // Technically there's still a race condition
    // where we check `m_is_completed` and it becomes
    // true right after we checked it's state. But it's
    // not a problem in our case because thread's state
    // access is still mutually exclusive, so the scheduler
    // will put us back into the queue after we yield.
    if (m_is_completed)
        return;

    // At this point the thread is blocked so we shouldn't
    // return here until the request is actually finished.
    Scheduler::the().yield();
}

void StorageDevice::AsyncRequest::complete()
{
    ASSERT(!m_is_completed);
    
    m_is_completed = true;
    m_blocker.unblock();
}

}