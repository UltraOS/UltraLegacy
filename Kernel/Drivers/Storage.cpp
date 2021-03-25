#include "Storage.h"

namespace kernel {

StorageDevice::AsyncRequest::AsyncRequest(Address virtual_address, LBARange lba_range, AsyncRequest::Type type)
    : m_blocker(*Thread::current())
    , m_virtual_address(virtual_address)
    , m_lba_range(lba_range)
    , m_type(type)
{
}

void StorageDevice::AsyncRequest::wait()
{
    if (m_is_completed)
        return;

    m_blocker.block();
}

void StorageDevice::AsyncRequest::complete()
{
    ASSERT(!m_is_completed);

    m_is_completed = true;
    m_blocker.unblock();
}

}