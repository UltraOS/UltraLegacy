#pragma once

#include "Common/Types.h"

#include "Core/Registers.h"

#include "InterruptHandler.h"

namespace kernel {

class SyscallDispatcher : public MonoInterruptHandler {
    MAKE_SINGLETON(SyscallDispatcher);

public:
    static void initialize();

    static constexpr u16 vector_number = 0x80;

    static constexpr u32 exit = 0;
    static constexpr u32 debug_log = 1;

private:
    void handle_interrupt(RegisterState&) override;

private:
    static SyscallDispatcher* s_instance;
};
}
