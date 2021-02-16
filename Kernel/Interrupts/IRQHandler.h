#pragma once

#include "Common/Types.h"

#include "Core/Registers.h"

#include "InterruptController.h"
#include "InterruptHandler.h"

namespace kernel {

class IRQHandler : public MonoInterruptHandler {
public:
    enum class Type {
        LEGACY,
        MSI,
        ANY,
        FIXED
    };

    IRQHandler(Type type, u16 vector = any_vector)
        : MonoInterruptHandler(type == Type::LEGACY ? legacy_irq_base + vector : vector, false)
        , m_type(type)
    {
    }

    u16 legacy_irq_number() const { return interrupt_vector() - legacy_irq_base; }
    Type irq_handler_type() const { return m_type; }

    virtual void enable_irq() { InterruptController::the().enable_irq_for(*this); }
    virtual void disable_irq() { InterruptController::the().disable_irq_for(*this); }

protected:
    virtual void handle_irq(RegisterState&) = 0;

private:
    void handle_interrupt(RegisterState& registers) override
    {
        handle_irq(registers);

        // We pass the legacy number because only PIC requires the irq number
        // to send an EOI, and if we're using PIC we're definitely not using
        // anything like MSIs/LAPIC timers, so it's kind of an invariant, although
        // I don't like that we have to assume anything at all :(
        InterruptController::the().end_of_interrupt(legacy_irq_number());
    }

private:
    Type m_type;
};

}
