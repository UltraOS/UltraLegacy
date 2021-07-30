#include "Pipe.h"
#include "Memory/MemoryManager.h"
#include "Memory/SafeOperations.h"

namespace kernel {

Pipe::Pipe(Side side, size_t capacity)
    : IOStream(IOStream::Type::PIPE, side == Side::READ ? IOMode::READONLY : IOMode::WRITEONLY)
    , m_side(side)
{
    ASSERT(capacity > 0);

    m_state = RefPtr<State>::create();
    m_state->capacity = Page::round_up(capacity);
    m_state->buffer = MemoryManager::the().allocate_kernel_private_anywhere("pipe", m_state->capacity);

    if (m_side == Side::READ)
        m_state->reader_count++;
    else
        m_state->writer_count++;
}

Pipe::Pipe(Side side, Pipe& other)
    : IOStream(IOStream::Type::PIPE, side == Side::READ ? IOMode::READONLY : IOMode::WRITEONLY)
    , m_state(other.m_state)
    , m_side(side)
{
    if (m_side == Side::READ)
        m_state->reader_count++;
    else
        m_state->writer_count++;
}

RefPtr<IOStream> Pipe::create(Side side, size_t capacity)
{
    return new Pipe(side, capacity);
}

ErrorOr<RefPtr<IOStream>> Pipe::clone(Side side)
{
    if (m_is_closed)
        return ErrorCode::STREAM_CLOSED;

    return RefPtr<IOStream>(new Pipe(side, *this));
}

ErrorOr<size_t> Pipe::read(void* buffer, size_t bytes)
{
    if (m_side == Side::WRITE)
        return ErrorCode::ACCESS_DENIED;

    LOCK_GUARD(m_state->lock);

    if (m_is_closed)
        return ErrorCode::STREAM_CLOSED;

    if (m_state->size == 0)
        return 0;

    size_t bytes_to_read = min(bytes, m_state->size);
    size_t final_bytes_read = bytes_to_read;

    u8* byte_buffer = reinterpret_cast<u8*>(buffer);
    const auto initial_read_offset = m_state->read_offset;
    const auto initial_buffer_size = m_state->size;

    while (bytes_to_read) {
        size_t this_read_bytes = min(bytes_to_read, m_state->capacity - m_state->read_offset);
        auto read_addr = Address(m_state->buffer->virtual_range().begin() + m_state->read_offset);

        if (!safe_copy_memory(read_addr.as_pointer<void>(), byte_buffer, this_read_bytes)) {
            m_state->read_offset = initial_read_offset;
            m_state->size = initial_buffer_size;
            return ErrorCode::MEMORY_ACCESS_VIOLATION;
        }

        bytes_to_read -= this_read_bytes;
        byte_buffer += this_read_bytes;
        m_state->read_offset += this_read_bytes;
        m_state->size -= this_read_bytes;

        if (m_state->read_offset == m_state->capacity)
            m_state->read_offset = 0;
    }

    while (!m_state->write_waiters.empty()) {
        auto& waiter = m_state->write_waiters.pop_front();
        waiter.unblock();
    }

    return final_bytes_read;
}

ErrorOr<size_t> Pipe::write(const void* buffer, size_t bytes)
{
    if (m_side == Side::READ)
        return ErrorCode::ACCESS_DENIED;

    LOCK_GUARD(m_state->lock);

    if (m_is_closed)
        return ErrorCode::STREAM_CLOSED;

    if ((m_state->capacity - m_state->size) == 0)
        return 0;

    size_t byte_to_write = min(bytes, m_state->capacity - m_state->size);
    size_t final_write_size = byte_to_write;

    const u8* byte_buffer = reinterpret_cast<const u8*>(buffer);
    const auto initial_write_offset = m_state->write_offset;
    const auto initial_buffer_size = m_state->size;

    while (byte_to_write) {
        size_t this_write_bytes = min(byte_to_write, m_state->capacity - m_state->write_offset);
        auto write_addr = Address(m_state->buffer->virtual_range().begin() + m_state->write_offset);

        if (!safe_copy_memory(byte_buffer, write_addr.as_pointer<void>(), this_write_bytes)) {
            m_state->write_offset = initial_write_offset;
            m_state->size = initial_buffer_size;
            return ErrorCode::MEMORY_ACCESS_VIOLATION;
        }

        byte_to_write -= this_write_bytes;
        byte_buffer += this_write_bytes;
        m_state->write_offset += this_write_bytes;
        m_state->size += this_write_bytes;

        if (m_state->write_offset == m_state->capacity)
            m_state->write_offset = 0;
    }

    while (!m_state->read_waiters.empty()) {
        auto& waiter = m_state->read_waiters.pop_front();
        waiter.unblock();
    }

    return final_write_size;
}

ErrorCode Pipe::close()
{
    LOCK_GUARD(m_state->lock);

    if (m_is_closed)
        return ErrorCode::STREAM_CLOSED;

    if (m_side == Side::READ) {
        auto new_count = --m_state->reader_count;

        // since there are no more readers, write waiters will never wake up otherwise.
        if (new_count == 0) {
            while (!m_state->write_waiters.empty()) {
                auto& waiter = m_state->write_waiters.pop_front();
                waiter.unblock();
            }
        }
    } else {
        auto new_count = --m_state->writer_count;

        // since there are no more writers, read waiters will never wake up otherwise.
        if (new_count == 0) {
            while (!m_state->read_waiters.empty()) {
                auto& waiter = m_state->read_waiters.pop_front();
                waiter.unblock();
            }
        }
    }

    m_is_closed = true;
    return ErrorCode::NO_ERROR;
}

ErrorOr<Blocker::Result> Pipe::block_until_readable()
{
    if (m_side == Side::WRITE)
        return ErrorCode::ACCESS_DENIED;

    auto* current_thread = Thread::current();
    IOBlocker blocker(*current_thread);

    {
        LOCK_GUARD(m_state->lock);

        if (m_is_closed)
            return ErrorCode::STREAM_CLOSED;
        if (m_state->size != 0)
            return Blocker::Result::UNBLOCKED;
        if (m_state->writer_count == 0)
            return ErrorCode::WOULD_BLOCK_FOREVER;

        m_state->read_waiters.insert_back(blocker);
    }

    auto res = blocker.block();

    {
        LOCK_GUARD(m_state->lock);
        if (blocker.is_on_a_list())
            blocker.pop_off();
    }

    return res;
}

ErrorOr<Blocker::Result> Pipe::block_until_writable()
{
    if (m_side == Side::READ)
        return ErrorCode::ACCESS_DENIED;

    auto* current_thread = Thread::current();
    IOBlocker blocker(*current_thread);

    {
        LOCK_GUARD(m_state->lock);

        if (m_is_closed)
            return ErrorCode::STREAM_CLOSED;
        if (m_state->capacity != m_state->size)
            return Blocker::Result::UNBLOCKED;
        if (m_state->reader_count == 0)
            return ErrorCode::WOULD_BLOCK_FOREVER;

        m_state->write_waiters.insert_back(blocker);
    }

    auto res = blocker.block();

    {
        LOCK_GUARD(m_state->lock);
        if (blocker.is_on_a_list())
            blocker.pop_off();
    }

    return res;
}

Pipe::State::~State()
{
    MemoryManager::the().free_virtual_region(*buffer);
}

}
