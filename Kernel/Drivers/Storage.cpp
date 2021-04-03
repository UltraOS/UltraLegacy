#include "Storage.h"

namespace kernel {

StorageDevice::Request::Request(Address virtual_address, OP op, Type type)
    : m_virtual_address(virtual_address)
    , m_op(op)
    , m_type(type)
{
}

StorageDevice::AsyncRequest::AsyncRequest(Address virtual_address, LBARange lba_range, Request::OP op)
    : Request(virtual_address, op, Type::ASYNC)
    , m_blocker(*Thread::current())
    , m_lba_range(lba_range)
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

StorageDevice::RamdiskRequest::RamdiskRequest(Address virtual_address, size_t byte_offset, size_t byte_count, OP op)
    : Request(virtual_address, op, Type::RAMDISK)
    , m_offset(byte_offset)
    , m_byte_count(byte_count)
{
}

}