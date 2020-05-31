#pragma once

#include "Core/Types.h"

#include "Common.h"
#include "ProgrammableInterruptController.h"
#include "InterruptRequestManager.h"

namespace kernel {
    class InterruptRequestHandler
    {
    public:
        InterruptRequestHandler(u16 irq_index)
            : m_irq_index(irq_index)
        {
            InterruptRequestManager::the().register_irq_handler(*this);
        }

        u16 irq_index() const { return m_irq_index; }

        virtual void on_irq(const RegisterState& registers) = 0;
        virtual void finialize_irq() { ProgrammableInterruptController::end_of_interrupt(irq_index()); }

        // TODO: add [virtual ~InterruptRequestHandler() = default;] once I have operator delete
    private:
        u16 m_irq_index;
    };
}
