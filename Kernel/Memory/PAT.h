#pragma once

#include "Common/Lock.h"
#include "Common/Macros.h"
#include "Common/Types.h"

namespace kernel {

class PAT {
    MAKE_SINGLETON(PAT);

public:
    static constexpr u32 msr_index = 0x277;

    enum class MemoryType : u8 {
        UNCACHEABLE = 0x00,
        WRITE_COMBINING = 0x01,
        WRITE_THROUGH = 0x04,
        WRITE_PROTECTED = 0x05,
        WRITE_BACK = 0x06,
        UNCACHED = 0x07
    };

    static PAT& the() { return s_instance; }

    void set_entry(u8 index, MemoryType);
    void synchronize();

private:
    InterruptSafeSpinLock m_lock;

    CPU::MSR m_state;
    bool m_is_supported { false };

    static PAT s_instance;
};
}
