#pragma once

#include "Common/Types.h"

#include "Common.h"
#include "PIC.h"
#include "IRQManager.h"

namespace kernel {
    class IRQHandler
    {
    public:
        IRQHandler(u16 irq_index)
            : m_irq_index(irq_index)
        {
            IRQManager::the().register_irq_handler(*this);
        }

        u16 irq_index() const { return m_irq_index; }

        virtual void on_irq(const RegisterState& registers) = 0;
        virtual void finialize_irq() { PIC::end_of_interrupt(irq_index()); }

        virtual ~IRQHandler() = default;
    private:
        u16 m_irq_index;
    };
}
