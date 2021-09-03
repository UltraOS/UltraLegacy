#include "DeferredIRQ.h"
#include "Multitasking/Process.h"

namespace kernel {

DeferredIRQManager* DeferredIRQManager::s_instance;

DeferredIRQHandler::DeferredIRQHandler()
{
    DeferredIRQManager::the().register_handler(*this);
}

DeferredIRQHandler::~DeferredIRQHandler()
{
    DeferredIRQManager::the().unregister_handler(*this);
}

void DeferredIRQHandler::deferred_invoke()
{
    DeferredIRQManager::the().request_invocation();
}

void DeferredIRQHandler::invoke()
{
    m_pending = false;
    handle_deferred_irq();
}

void DeferredIRQManager::request_invocation()
{
    LOCK_GUARD(m_state_lock);
    m_blocker->unblock();
}

void DeferredIRQManager::register_handler(DeferredIRQHandler& handler)
{
    LOCK_GUARD(m_state_lock);

    ASSERT(!m_handlers.contains(&handler));
    m_handlers.emplace(&handler);
}

void DeferredIRQManager::unregister_handler(DeferredIRQHandler& handler)
{
    LOCK_GUARD(m_state_lock);

    ASSERT(m_handlers.contains(&handler));
    m_handlers.remove(&handler);
}

void DeferredIRQManager::do_run_handlers()
{
    auto& instance = the();

    for (;;) {
        IRQBlocker irq_blocker(*Thread::current());

        {
            LOCK_GUARD(instance.m_state_lock);
            instance.m_blocker = &irq_blocker;

            for (auto& handler : instance.m_handlers) {
                while (handler->is_pending())
                    handler->invoke();
            }
        }

        auto res = irq_blocker.block();
        ASSERT(res == Blocker::Result::UNBLOCKED);

        {
            LOCK_GUARD(instance.m_state_lock);
            instance.m_blocker = nullptr;
        }
    }
}

void DeferredIRQManager::initialize()
{
    ASSERT(s_instance == nullptr);
    s_instance = new DeferredIRQManager;

    Process::create_supervisor(&do_run_handlers, "deferred_irq"_sv);
}

DeferredIRQManager& DeferredIRQManager::the()
{
    ASSERT(s_instance != nullptr);

    return *s_instance;
}

}
