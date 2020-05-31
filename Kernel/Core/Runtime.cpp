#include "Logger.h"
#include "Macros.h"

extern "C" void __cxa_pure_virtual()
{
    kernel::error() << "A pure virtual call!";
    hang();
}

using global_constructor_t = void(*)();
global_constructor_t global_constructors_begin;
global_constructor_t global_constructors_end;

namespace runtime {

    void init_global_objects()
    {
        for (global_constructor_t* ctor = &global_constructors_begin; ctor < &global_constructors_end; ++ctor)
            (*ctor)();
    }
}
