#include "Common/Logger.h"

#include "IRQHandler.h"
#include "IDT.h"
#include "IRQManager.h"
#include "PIC.h"

DEFINE_IRQ_HANDLER(0)
DEFINE_IRQ_HANDLER(1)
DEFINE_IRQ_HANDLER(2)
DEFINE_IRQ_HANDLER(3)
DEFINE_IRQ_HANDLER(4)
DEFINE_IRQ_HANDLER(5)
DEFINE_IRQ_HANDLER(6)
DEFINE_IRQ_HANDLER(7)
DEFINE_IRQ_HANDLER(8)
DEFINE_IRQ_HANDLER(9)
DEFINE_IRQ_HANDLER(10)
DEFINE_IRQ_HANDLER(11)
DEFINE_IRQ_HANDLER(12)
DEFINE_IRQ_HANDLER(13)
DEFINE_IRQ_HANDLER(14)
DEFINE_IRQ_HANDLER(15)

namespace kernel {

    IRQManager IRQManager::s_instance;

    IRQManager::IRQManager()
        : m_handlers{}
    {
        PIC::remap(irq_base_index + 1);
    }

    IRQManager& IRQManager::the()
    {
        return s_instance;
    }

    void IRQManager::install()
    {
        IDT::the().register_interrupt_handler(irq_base_index + 0,  irq0_handler)
                  .register_interrupt_handler(irq_base_index + 1,  irq1_handler)
                  .register_interrupt_handler(irq_base_index + 2,  irq2_handler)
                  .register_interrupt_handler(irq_base_index + 3,  irq3_handler)
                  .register_interrupt_handler(irq_base_index + 4,  irq4_handler)
                  .register_interrupt_handler(irq_base_index + 5,  irq5_handler)
                  .register_interrupt_handler(irq_base_index + 6,  irq6_handler)
                  .register_interrupt_handler(irq_base_index + 7,  irq7_handler)
                  .register_interrupt_handler(irq_base_index + 8,  irq8_handler)
                  .register_interrupt_handler(irq_base_index + 9,  irq9_handler)
                  .register_interrupt_handler(irq_base_index + 10, irq10_handler)
                  .register_interrupt_handler(irq_base_index + 11, irq11_handler)
                  .register_interrupt_handler(irq_base_index + 12, irq12_handler)
                  .register_interrupt_handler(irq_base_index + 13, irq13_handler)
                  .register_interrupt_handler(irq_base_index + 14, irq14_handler)
                  .register_interrupt_handler(irq_base_index + 15, irq15_handler);
    }

    bool IRQManager::has_subscriber(u16 request_number)
    {
        return m_handlers[request_number];
    }

    bool IRQManager::is_spurious(u16 request_number)
    {
        if (request_number == spurious_master || request_number == spurious_slave)
            return !PIC::is_irq_being_serviced(request_number);

        return false;
    }

    void IRQManager::handle_spurious_irq(u16 request_number)
    {
        if (request_number == spurious_slave)
            PIC::end_of_interrupt(request_number, true);
    }

    void IRQManager::irq_handler(u16 request_number, RegisterState registers)
    {
        if (is_spurious(request_number))
        {
            warning() << "Spurious IRQ " << request_number << "!";
            handle_spurious_irq(request_number);
            return;
        }

        if (!the().has_subscriber(request_number))
        {
            error() << "Unexpected IRQ " << request_number << " with no receiver!";
            hang();
        }

        the().m_handlers[request_number]->on_irq(registers);
        the().m_handlers[request_number]->finialize_irq();
    }

    void IRQManager::register_irq_handler(IRQHandler& handler)
    {
        if (handler.irq_index() > max_irq_index)
        {
            error() << "Bad IRQ index " << handler.irq_index();
            hang();
        }

        m_handlers[handler.irq_index()] = &handler;
    }
}
