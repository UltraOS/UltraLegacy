#pragma once

#include "Core/CPU.h"

namespace kernel::Interrupts {

inline bool are_enabled()
{
    return CPU::flags() & CPU::FLAGS::INTERRUPTS;
}

inline void enable()
{
    sti();
}

inline void disable()
{
    cli();
}

class ScopedDisabler {
public:
    ScopedDisabler() : m_should_enable(are_enabled()) { disable(); }

    ~ScopedDisabler()
    {
        if (m_should_enable)
            enable();
    }

private:
    bool m_should_enable { false };
};
}
