#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"

#include "Core/Registers.h"

namespace kernel {

class IPICommunicator {
    MAKE_STATIC(IPICommunicator);

public:
    static constexpr u16 vector_number = 254;

    static void install();

    static void send_ipi(u8 dest);

    static void hang_all_cores();

private:
    static void on_ipi(RegisterState*) USED;
};
}
