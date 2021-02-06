#pragma once

#include "Common/Macros.h"
#include "Core/Runtime.h"

namespace kernel {

class Access {
    MAKE_INHERITABLE_SINGLETON(Access) = default;
public:
    static void detect();

    static Access& the()
    {
        ASSERT(s_instance != nullptr);
        return *s_instance;
    }

private:
    static Access* s_instance;
};

}
