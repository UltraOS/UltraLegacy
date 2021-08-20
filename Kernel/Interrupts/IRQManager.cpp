#include "IRQManager.h"
#include "InterruptController.h"
#include "Utilities.h"

namespace kernel {

IRQManager* IRQManager::s_instance;

void IRQManager::register_handler(IRQHandler& handler, u16 requested_vector)
{
    LOCK_GUARD(m_lock);

    if (requested_vector == any_vector) {
        ASSERT(handler.irq_handler_type() != IRQHandler::Type::LEGACY);

        auto vec = allocate_one();
        handler.set_vector_number(vec);
        m_vec_to_handlers[vec].insert_back(handler);
        return;
    }

    ASSERT(requested_vector < 256);

    if (handler.irq_handler_type() == IRQHandler::Type::LEGACY)
        requested_vector += legacy_irq_base;

    ASSERT(requested_vector >= legacy_irq_base);

    auto& handlers = m_vec_to_handlers[requested_vector];

    if (handlers.empty())
        allocate_one(requested_vector);

    handlers.insert_back(handler);
    handler.set_vector_number(requested_vector);
}

void IRQManager::unregister_handler(IRQHandler& handler)
{
    LOCK_GUARD(m_lock);

    handler.pop_off();

    auto& other_handlers = m_vec_to_handlers[handler.interrupt_vector()];
    if (!other_handlers.empty())
        return;

    free_one(handler.interrupt_vector());
}

void IRQManager::handle_interrupt(RegisterState& registers)
{
    // FIXME: technically there's a race condition here if devices are
    //        registered and removed at runtime after initialization,
    //        but we don't really support that right now so it's not that important.

    auto vec = registers.interrupt_number;

    auto& handlers = m_vec_to_handlers[vec];
    ASSERT(!handlers.empty());

    for (auto& handler : handlers)
        handler.handle_irq(registers);

    // We pass the legacy number because only PIC requires the irq number
    // to send an EOI, and if we're using PIC we're definitely not using
    // anything like MSIs/LAPIC timers, so it's kind of an invariant, although
    // I don't like that we have to assume anything at all :(
    InterruptController::primary().end_of_interrupt(registers.interrupt_number - legacy_irq_base);
}

}
