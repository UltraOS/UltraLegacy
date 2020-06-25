#pragma once

#include "Common/Types.h"
#include "Interrupts/Common.h"

namespace kernel {

class Scheduler
{
public:
    static void inititalize();
    Scheduler& the();

    // returns a new esp in eax
    static ptr_t on_tick(RegisterState);

private:
    Scheduler() = default;

private:
    static Scheduler* s_instance;
};
}
