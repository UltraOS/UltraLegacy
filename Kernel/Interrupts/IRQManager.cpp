#include "Common/Logger.h"

#include "IDT.h"
#include "IRQHandler.h"
#include "IRQManager.h"
#include "InterruptController.h"
#include "LAPIC.h"

IRQ_HANDLER_SYMBOL(0);
IRQ_HANDLER_SYMBOL(1);
IRQ_HANDLER_SYMBOL(2);
IRQ_HANDLER_SYMBOL(3);
IRQ_HANDLER_SYMBOL(4);
IRQ_HANDLER_SYMBOL(5);
IRQ_HANDLER_SYMBOL(6);
IRQ_HANDLER_SYMBOL(7);
IRQ_HANDLER_SYMBOL(8);
IRQ_HANDLER_SYMBOL(9);
IRQ_HANDLER_SYMBOL(10);
IRQ_HANDLER_SYMBOL(11);
IRQ_HANDLER_SYMBOL(12);
IRQ_HANDLER_SYMBOL(13);
IRQ_HANDLER_SYMBOL(14);
IRQ_HANDLER_SYMBOL(15);
IRQ_HANDLER_SYMBOL(253); // LAPIC timer per core
IRQ_HANDLER_SYMBOL(255); // APIC spurious vector

namespace kernel {

IRQHandler* IRQManager::s_handlers[IRQManager::entry_count];

void IRQManager::install()
{
    IDT::the()
        .register_interrupt_handler(irq_base_index + 0, irq0_handler)
        .register_interrupt_handler(irq_base_index + 1, irq1_handler)
        .register_interrupt_handler(irq_base_index + 2, irq2_handler)
        .register_interrupt_handler(irq_base_index + 3, irq3_handler)
        .register_interrupt_handler(irq_base_index + 4, irq4_handler)
        .register_interrupt_handler(irq_base_index + 5, irq5_handler)
        .register_interrupt_handler(irq_base_index + 6, irq6_handler)
        .register_interrupt_handler(irq_base_index + 7, irq7_handler)
        .register_interrupt_handler(irq_base_index + 8, irq8_handler)
        .register_interrupt_handler(irq_base_index + 9, irq9_handler)
        .register_interrupt_handler(irq_base_index + 10, irq10_handler)
        .register_interrupt_handler(irq_base_index + 11, irq11_handler)
        .register_interrupt_handler(irq_base_index + 12, irq12_handler)
        .register_interrupt_handler(irq_base_index + 13, irq13_handler)
        .register_interrupt_handler(irq_base_index + 14, irq14_handler)
        .register_interrupt_handler(irq_base_index + 15, irq15_handler)
        .register_interrupt_handler(LAPIC::Timer::irq_number, irq253_handler)
        .register_interrupt_handler(LAPIC::spurious_irq_index, irq255_handler);
}

bool IRQManager::has_subscriber(u16 request_number)
{
    return s_handlers[request_number];
}

void IRQManager::irq_handler(RegisterState* registers)
{
    const auto& request_number = registers->irq_number;

    if (InterruptController::the().is_spurious(request_number)) {
        warning() << "IRQManager: Spurious IRQ " << request_number << "!";
        InterruptController::the().handle_spurious_irq(request_number);
        return;
    }

    if (!has_subscriber(request_number)) {
        StackStringBuilder error_string;
        error_string << "IRQManager: Unexpected IRQ " << request_number << " with no receiver!";
        runtime::panic(error_string.data());
    }

    s_handlers[request_number]->handle_irq(*registers);
    s_handlers[request_number]->finalize_irq();
}

void IRQManager::register_irq_handler(IRQHandler& handler)
{
    if (handler.irq_index() >= entry_count) {
        StackStringBuilder error_string;
        error_string << "IRQManager: tried to register out of bounds handler " << handler.irq_index();
        runtime::panic(error_string.data());
    }

    s_handlers[handler.irq_index()] = &handler;
}

void IRQManager::unregister_irq_handler(IRQHandler& handler)
{
    if (handler.irq_index() >= entry_count || !s_handlers[handler.irq_index()]) {
        StackStringBuilder error_string;
        error_string << "IRQManager: tried to unregister non-existant handler " << handler.irq_index();
        runtime::panic(error_string.data());
    }

    s_handlers[handler.irq_index()] = nullptr;
}
}
