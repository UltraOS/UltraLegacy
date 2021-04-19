#include "FileDescription.h"

namespace kernel {

FileDescription::FileDescription(File& file, Mode mode)
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

size_t FileDescription::write(void* buffer, size_t size)
{
    LOCK_GUARD(m_lock);

    if (m_is_closed)
        return 0;

    auto written_bytes = m_file.write(buffer, m_offset, size);
    m_offset += written_bytes;

    return written_bytes;
}

void FileDescription::set_offset(size_t offset)
{
    LOCK_GUARD(m_lock);
    m_offset = offset;
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