#pragma once

#include "Common/Types.h"
#include "Core/ErrorCode.h"
#include "Multitasking/Mutex.h"
#include "Multitasking/Blocker.h"

#include <Shared/IO.h>

namespace kernel {

enum class IOMode : u32 {
#define IO_MODE(name, bit) name = 1 << bit,
    ENUMERATE_IO_MODES
#undef IO_MODE
};

enum class SeekMode : u32 {
#define SEEK_MODE(name) name,
    ENUMERATE_SEEK_MODES
#undef SEEK_MODE
};


// Behavior expected from the functions:
// - read()
// Non blocking, reads up to 'size' bytes to the buffer.
//
// - write()
// Non blocking, either writes 'size' bytes or nothing, but is allowed to
// write less bytes in case for example there isn't enough space left on disk
// and the io stream is a FileIterator.
//
// - seek()
// Sets the current read/write offset in the stream, if applicable.
//
// - block_until_readable/writable()
// Blocks the current thread until there is at least 1 byte to be read/write,
// calls to read/write() are not guaranteed to succeed after this function returns, use a loop.
//
// - can_read/write_without_blocking()
// Indicates whether a call to read/write is likely to succeed, not guaranteed to work.
//
// - close()
// Releases the underlying resources, e.g closes the file in case of FileIterator.
// All further calls to read/write/etc return the STREAM_CLOSED error code.
//
// - is_closed()
// Indicates whether a call to close() has been made before for this IO stream. Calls to read/write
// are not guaranteed to succeed after checking this.

class IOStream {
    MAKE_NONMOVABLE(IOStream);
    MAKE_NONCOPYABLE(IOStream);
public:
    enum class Type {
        FILE_ITERATOR,
        PIPE,
        SOCKET,
    };

    IOStream(Type type, IOMode mode)
        : m_type(type)
        , m_io_mode(mode)
    {
    }

    virtual ErrorOr<size_t> read(void* buffer, size_t size) = 0;
    virtual ErrorOr<size_t> write(const void* buffer, size_t size) = 0;
    virtual ErrorOr<size_t> seek(size_t, SeekMode) { return ErrorCode::UNSUPPORTED; }

    virtual ErrorOr<Blocker::Result> block_until_readable() { return ErrorCode::UNSUPPORTED; };
    virtual ErrorOr<Blocker::Result> block_until_writable() { return ErrorCode::UNSUPPORTED; };

    virtual bool can_read_without_blocking() { return true; }
    virtual bool can_write_without_blocking() { return true; }

    virtual ErrorCode close() = 0;
    [[nodiscard]] bool is_closed() const { return m_is_closed; }

    [[nodiscard]] IOMode io_mode() const { return m_io_mode; }
    [[nodiscard]] Type type() const { return m_type; }

    virtual ~IOStream() { ASSERT(m_is_closed); }

protected:
    Type m_type;
    IOMode m_io_mode;
    bool m_is_closed { false };
};

}
