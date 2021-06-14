#include "FileDescription.h"

namespace kernel {

FileDescription::FileDescription(File& file, FileMode mode)
    : m_file(file)
    , m_mode(mode)
{
}

size_t FileDescription::read(void* buffer, size_t size)
{
    LOCK_GUARD(m_lock);

    if (m_is_closed)
        return 0;

    auto read_bytes = m_file.read(buffer, m_offset, size);
    m_offset += read_bytes;

    return read_bytes;
}

size_t FileDescription::write(const void* buffer, size_t size)
{
    LOCK_GUARD(m_lock);

    if (m_is_closed)
        return 0;

    auto written_bytes = m_file.write(buffer, m_offset, size);
    m_offset += written_bytes;

    return written_bytes;
}

ErrorCode FileDescription::set_offset(size_t offset, SeekMode mode)
{
    LOCK_GUARD(m_lock);

    switch (mode) {
    case SeekMode::BEGINNING:
        m_offset = offset;
        return ErrorCode::NO_ERROR;
    case SeekMode::CURRENT:
        m_offset += offset;
        return ErrorCode::NO_ERROR;
    case SeekMode::END:
        m_offset = m_file.size();
        m_offset += offset;
        return ErrorCode::NO_ERROR;
    default:
        return ErrorCode::INVALID_ARGUMENT;
    }
}

void FileDescription::mark_as_closed()
{
    LOCK_GUARD(m_lock);
    ASSERT(m_is_closed == false);
    m_is_closed = true;
}

FileDescription::~FileDescription()
{
    ASSERT(m_is_closed == true);
}

}