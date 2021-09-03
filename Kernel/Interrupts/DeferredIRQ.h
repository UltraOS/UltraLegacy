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
    [[nodiscard]] bool is_pending() const { return m_pending; }

    virtual void handle_deferred_irq() = 0;

private:
    friend class DeferredIRQManager;
    void invoke();

private:
    bool m_pending { false };
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
    InterruptSafeSpinLock m_state_lock;

    // We cannot use atomic here because as soon as blocker goes
    // out of scope it becomes invalid. And there's no guarantees
    // that the blocker loaded from an atomic variable remains valid
    // for the duration of the call.
    Blocker* m_blocker { nullptr };

    Set<DeferredIRQHandler*> m_handlers;

    static DeferredIRQManager* s_instance;
};

}
