#include "PAT.h"
#include "Common/Logger.h"

namespace kernel {

PAT PAT::s_instance;

PAT::PAT()
{
    static constexpr u32 pat_support_bit = SET_BIT(16);

    m_is_supported = CPU::ID(0x01).d & pat_support_bit;

    if (m_is_supported)
        m_state = CPU::MSR::read(msr_index);
}

void PAT::set_entry(u8 index, MemoryType type)
{
    ASSERT(index > 3 && index < 8);

    if (!m_is_supported) {
        warning() << "PAT: cannot set memory type, feature unsupported.";
        return;
    }

    LOCK_GUARD(m_lock);

    // Trying not to break strict aliasing :)
    u8 msr_as_byte_array[sizeof(m_state.upper)] {};
    copy_memory(&m_state.upper, msr_as_byte_array, sizeof(m_state.upper));

    msr_as_byte_array[index - 4] = static_cast<u8>(type);

    copy_memory(msr_as_byte_array, &m_state.upper, sizeof(m_state.upper));

    m_state.write(msr_index);
}

void PAT::synchronize()
{
    if (!m_is_supported)
        return;

    LOCK_GUARD(m_lock);

    m_state.write(msr_index);
}
}
