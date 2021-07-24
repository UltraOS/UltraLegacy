#pragma once

#include "IOStream.h"
#include "Memory/MemoryManager.h"

namespace kernel {

class Pipe : public IOStream {
public:
    enum class Side {
        READ,
        WRITE
    };

    static RefPtr<IOStream> create(Side, size_t capacity = Page::size);
    ErrorOr<RefPtr<IOStream>> clone(Side);

    ErrorOr<size_t> read(void* buffer, size_t bytes) override;
    ErrorOr<size_t> write(const void* buffer, size_t bytes) override;
    ErrorOr<Blocker::Result> block_until_readable() override;
    ErrorOr<Blocker::Result> block_until_writable() override;

    ErrorCode close() override;

private:
    Pipe(Side, Pipe&);
    Pipe(Side, size_t capacity);

private:
    struct State {
        size_t reader_count { 0 };
        size_t writer_count { 0 };

        size_t read_offset { 0 };
        size_t write_offset { 0 };

        size_t capacity { 0 };
        size_t size { 0 };

        InterruptSafeSpinLock lock;
        List<IOBlocker> read_waiters;
        List<IOBlocker> write_waiters;
        MemoryManager::VR buffer;

        ~State();
    };

    RefPtr<State> m_state;
    Side m_side;
};

}
