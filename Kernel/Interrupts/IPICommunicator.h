#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"

#include "Core/Registers.h"
#include "InterruptHandler.h"

namespace kernel {

class IPICommunicator : public MonoInterruptHandler {
    MAKE_SINGLETON(IPICommunicator);
public:
    static constexpr u16 vector_number = 254;

    static void initialize();

    static IPICommunicator& the()
    {
        ASSERT(s_instance != nullptr);
        return *s_instance;
    }

    void send_ipi(u8 dest);
    void hang_all_cores();

    void handle_interrupt(RegisterState&) override;

private:
    static IPICommunicator* s_instance;
};
}
