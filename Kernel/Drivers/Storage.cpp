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

ErrorCode StorageDevice::AsyncRequest::wait()
{
    if (m_is_completed)
        return result();

    m_blocker.block();
    return result();
}

void StorageDevice::AsyncRequest::complete(ErrorCode code)
{
    ASSERT(!m_is_completed);

    set_result(code);
    m_blocker.unblock();
    m_is_completed = true;
}

StorageDevice::RamdiskRequest::RamdiskRequest(Address virtual_address, size_t byte_offset, size_t byte_count, OP op)
    : Request(virtual_address, op, Type::RAMDISK)
    , m_offset(byte_offset)
    , m_byte_count(byte_count)
{
}

void StorageDevice::RamdiskRequest::complete(ErrorCode code)
{
    set_result(code);
}

}
