#pragma once

#include "Common/Macros.h"

namespace kernel {

    class InterruptDisabler
    {
    public:
        InterruptDisabler()  { cli(); }
        ~InterruptDisabler() { sti(); }
    };
}
