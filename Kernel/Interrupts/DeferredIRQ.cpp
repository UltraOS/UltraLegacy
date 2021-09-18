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
    m_pending_count.fetch_add(1, MemoryOrder::ACQ_REL);
    DeferredIRQManager::the().request_invocation();
}

void DeferredIRQManager::request_invocation()
{
    LOCK_GUARD(m_blocker_access_lock);
    if (m_blocker)
        m_blocker->unblock();
}

void DeferredIRQManager::register_handler(DeferredIRQHandler& handler)
{
    LOCK_GUARD(m_handlers_modification_lock);

    ASSERT(!m_handlers.contains(&handler));
    m_handlers.emplace(&handler);
}

void DeferredIRQManager::unregister_handler(DeferredIRQHandler& handler)
{
    LOCK_GUARD(m_handlers_modification_lock);

    ASSERT(m_handlers.contains(&handler));
    m_handlers.remove(&handler);
}

void DeferredIRQManager::do_run_handlers()
{
    auto* current_thread = Thread::current();
    current_thread->set_invulnerable(true);

    auto& instance = the();

    // The reason this loop is so weird is that we don't want to
    // ever go into a blocked state whenever there's a pending deferred IRQ.
    // With this setup it's currently impossible since we set the blocker before
    // actually running the handlers. If we get an invocation request before
    // calling block(), the block call itself will instantly return without
    // actually blocking the thread, and we will go to the next iteration.
    for (;;) {
        IRQBlocker irq_blocker(*current_thread);

        {
            LOCK_GUARD(instance.m_blocker_access_lock);
            instance.m_blocker = &irq_blocker;
        }

        {
            LOCK_GUARD(instance.m_handlers_modification_lock);
            for (auto& handler : instance.m_handlers) {
                while (handler->is_pending())
                    handler->invoke();
            }
        }

        auto res = irq_blocker.block();
        ASSERT(res == Blocker::Result::UNBLOCKED);

        {
            LOCK_GUARD(instance.m_blocker_access_lock);
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
