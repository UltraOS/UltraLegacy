#include "FileIterator.h"
#include "VFS.h"

namespace kernel {

RefPtr<IOStream> FileIterator::create(File& file, IOMode mode)
{
    return static_cast<IOStream*>(new FileIterator(file, mode));
}

FileIterator::FileIterator(File& file, IOMode mode)
    : IOStream(IOStream::Type::FILE_ITERATOR, mode)
    , m_file(file)
{
}

ErrorOr<size_t> FileIterator::read(void* buffer, size_t size)
{
    LOCK_GUARD(m_lock);

    if (m_is_closed)
        return ErrorCode::STREAM_CLOSED;

    auto read_bytes_or_error = m_file.read(buffer, m_offset, size);
    if (!read_bytes_or_error.is_error())
        m_offset += read_bytes_or_error.value();

    return read_bytes_or_error;
}

ErrorOr<size_t> FileIterator::write(const void* buffer, size_t size)
{
    LOCK_GUARD(m_lock);

    if (m_is_closed)
        return ErrorCode::STREAM_CLOSED;

    auto written_bytes_or_error = m_file.write(buffer, m_offset, size);
    if (!written_bytes_or_error.is_error())
        m_offset += written_bytes_or_error.value();

    return written_bytes_or_error;
}

ErrorOr<size_t> FileIterator::seek(size_t offset, SeekMode mode)
{
    LOCK_GUARD(m_lock);

    if (m_is_closed)
        return ErrorCode::STREAM_CLOSED;

    switch (mode) {
    case SeekMode::BEGINNING:
        m_offset = offset;
        return m_offset;
    case SeekMode::CURRENT:
        m_offset += offset;
        return m_offset;
    case SeekMode::END:
        m_offset = m_file.size();
        m_offset += offset;
        return m_offset;
    default:
        return ErrorCode::INVALID_ARGUMENT;
    }
}

ErrorCode FileIterator::truncate(size_t bytes)
{
    LOCK_GUARD(m_lock);
    if (m_is_closed)
        return ErrorCode::STREAM_CLOSED;

    return m_file.truncate(bytes);
}

ErrorCode FileIterator::close()
{
    LOCK_GUARD(m_lock);

    if (m_is_closed)
        return ErrorCode::STREAM_CLOSED;

    auto& fs = m_file.fs();
    auto code = fs.close(m_file);
    m_is_closed = true;

    return code;
}

}
