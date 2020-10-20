#pragma once

#include "Common/Types.h"

#include "Core/Registers.h"

#include "IRQManager.h"
#include "InterruptController.h"

namespace kernel {

class IRQHandler {
    MAKE_NONCOPYABLE(IRQHandler);

public:
    IRQHandler(u16 irq_index) : m_irq_index(irq_index) { IRQManager::register_irq_handler(*this); }

    u16 irq_index() const { return m_irq_index; }

    virtual void handle_irq(const RegisterState& registers) = 0;
    virtual void finalize_irq() { InterruptController::the().end_of_interrupt(irq_index()); }

    virtual void enable_irq() { InterruptController::the().enable_irq(irq_index()); }
    virtual void disable_irq() { InterruptController::the().disable_irq(irq_index()); }

    virtual ~IRQHandler() { IRQManager::unregister_irq_handler(*this); }

private:
    u16 m_irq_index;
};
}
