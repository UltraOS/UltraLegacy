#pragma once

#include "Multitasking/Blocker.h"
#include "Common/Lock.h"
#include "Common/Set.h"

namespace kernel {

class DeferredIRQHandler {
public:
    DeferredIRQHandler();
    ~DeferredIRQHandler();

    void deferred_invoke();
    [[nodiscard]] bool is_pending() const { return m_pending_count.load(MemoryOrder::ACQUIRE); }

    virtual bool handle_deferred_irq() = 0;

private:
    friend class DeferredIRQManager;
    void invoke()
    {
        auto new_count = m_pending_count.fetch_subtract(1, MemoryOrder::ACQ_REL);
        ASSERT(new_count);

        handle_deferred_irq();
    }

private:
    Atomic<size_t> m_pending_count { 0 };
};

class DeferredIRQManager {
public:
    static void initialize();
    static DeferredIRQManager& the();

    void request_invocation();

    void register_handler(DeferredIRQHandler&);
    void unregister_handler(DeferredIRQHandler&);

private:
    static void do_run_handlers();

private:
    InterruptSafeSpinLock m_blocker_access_lock;

    // NOTE: not interrupt safe
    SpinLock m_handlers_modification_lock;

    // We cannot use atomic here because as soon as blocker goes
    // out of scope it becomes invalid. And there's no guarantees
    // that the blocker loaded from an atomic variable remains valid
    // for the duration of the call.
    Blocker* m_blocker { nullptr };

    Set<DeferredIRQHandler*> m_handlers;

    static DeferredIRQManager* s_instance;
};

}
