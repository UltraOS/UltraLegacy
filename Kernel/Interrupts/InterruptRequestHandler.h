#pragma once

#include "Core/Types.h"

#include "Common.h"

namespace kernel {
    class InterruptRequestHandler
    {
    public:
        InterruptRequestHandler(u16 irq_index)
            : m_irq_index(irq_index)
        {
        }

        u16 irq_index() const { return m_irq_index; }

        virtual void on_irq(RegisterState& registers) = 0;

        // TODO: add [virtual ~InterruptRequestHandler() = default;] once I have operator delete
    private:
        u16 m_irq_index;
    };
}
