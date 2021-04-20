#pragma once

#include "Common/Types.h"
#include "Common/List.h"

#include "Core/Registers.h"

#include "InterruptController.h"
#include "InterruptHandler.h"
#include "Utilities.h"

namespace kernel {

class IRQHandler : public StandaloneListNode<IRQHandler> {
public:
    enum class Type {
        LEGACY,
        MSI,
        ANY,
        FIXED
    };

    IRQHandler(Type type, u16 vector = any_vector);

    u16 interrupt_vector() const { return m_interrupt_vector; }
    u16 legacy_irq_number() const { return interrupt_vector() - legacy_irq_base; }
    Type irq_handler_type() const { return m_type; }

    virtual void enable_irq() { InterruptController::primary().enable_irq_for(*this); }
    virtual void disable_irq() { InterruptController::primary().disable_irq_for(*this); }

    virtual bool is_non_msi_pci() const { return false; }

    virtual bool handle_irq(RegisterState&) = 0;

    virtual ~IRQHandler();

private:
    friend class IRQManager;
    void set_vector_number(u16 vector) { m_interrupt_vector = vector; }

private:
    Type m_type;
    u16 m_interrupt_vector { 0 };
};

class NonMSIPCIIRQHandler : public IRQHandler {
public:
    NonMSIPCIIRQHandler(Optional<PCIIRQ> pci_irq, Type type, u16 vector = any_vector)
        : IRQHandler(type, vector)
    {
        // If PCI IRQ is not specified we assume LEGACY mode is enabled
        if (!pci_irq) {
            ASSERT(type == Type::LEGACY);
            ASSERT(InterruptController::is_legacy_mode());
            ASSERT(vector != any_vector);
        }
    }

    virtual bool is_non_msi_pci() { return true; }
    const Optional<PCIIRQ>& routing_info() const { return m_pci_irq; }

private:
    Optional<PCIIRQ> m_pci_irq;
};

}
