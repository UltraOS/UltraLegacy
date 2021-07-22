#pragma once

#include "Common/Types.h"
#include "File.h"
#include "IOStream.h"

namespace kernel {

class FileIterator : public IOStream {
public:
    static RefPtr<IOStream> create(File& file, IOMode mode);

    ErrorOr<size_t> read(void* buffer, size_t size) override;
    ErrorOr<size_t> write(const void* buffer, size_t size) override;
    ErrorOr<size_t> seek(size_t offset, SeekMode mode) override;
    ErrorCode truncate(size_t bytes);

    ErrorCode close() override;

    File& underlying_file() { return m_file; }

private:
    FileIterator(File& file, IOMode mode);

private:
    File& m_file;
    Mutex m_lock;
    size_t m_offset { 0 };
};

}
