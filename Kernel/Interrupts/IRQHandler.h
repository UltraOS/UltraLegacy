#pragma once

#include "Common/Types.h"

#include "Common.h"
#include "IRQManager.h"
#include "PIC.h"

namespace kernel {

class IRQHandler {
public:
    IRQHandler(u16 irq_index) : m_irq_index(irq_index) { IRQManager::register_irq_handler(*this); }

    u16 irq_index() const { return m_irq_index; }

    virtual void on_irq(const RegisterState& registers) = 0;
    virtual void finialize_irq() { PIC::end_of_interrupt(irq_index()); }

    virtual void enable_irq() { PIC::enable_irq(irq_index()); }
    virtual void disable_irq() { PIC::disable_irq(irq_index()); }

    virtual ~IRQHandler() { IRQManager::unregister_irq_handler(*this); }

private:
    u16 m_irq_index;
};
}
