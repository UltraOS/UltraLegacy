#pragma once

#include "Common/List.h"
#include "Common/Lock.h"
#include "Common/Map.h"
#include "IDT.h"
#include "IRQHandler.h"
#include "InterruptHandler.h"

namespace kernel {

class IRQManager : public DynamicInterruptHandler {
    MAKE_SINGLETON(IRQManager) = default;

public:
    static void initialize()
    {
        ASSERT(s_instance == nullptr);
        s_instance = new IRQManager;
    }

    static IRQManager& the()
    {
        ASSERT(s_instance != nullptr);
        return *s_instance;
    }

    void register_handler(IRQHandler&, u16 requested_vector);
    void unregister_handler(IRQHandler&);

private:
    void handle_interrupt(RegisterState& registers) override;

private:
    InterruptSafeSpinLock m_lock;

    List<IRQHandler> m_vec_to_handlers[IDT::entry_count] {};
    static IRQManager* s_instance;
};

}