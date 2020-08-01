#pragma once

#include "Common/DynamicArray.h"
#include "Common/String.h"
#include "Common/Types.h"

namespace kernel {

class CPU {
public:
    enum class FLAGS : size_t {
        CARRY      = SET_BIT(0),
        PARITY     = SET_BIT(2),
        ADJUST     = SET_BIT(4),
        ZERO       = SET_BIT(6),
        SIGN       = SET_BIT(7),
        INTERRUPTS = SET_BIT(9),
        DIRECTION  = SET_BIT(10),
        OVERFLOW   = SET_BIT(11),
        CPUID      = SET_BIT(21),
    };

    friend bool operator&(FLAGS l, FLAGS r) { return static_cast<size_t>(l) & static_cast<size_t>(r); }

    static FLAGS flags();

    static bool supports_smp();

    static void start_all_processors();

private:
    static void ap_entrypoint() USED;
};
}
