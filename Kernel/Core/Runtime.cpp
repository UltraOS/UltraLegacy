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

constexpr size_t magic_length = 13;
extern const char magic_string[magic_length];

namespace kernel::runtime {

    void ensure_loaded_correctly()
    {
        if (magic_string[0]  == 'M' &&
            magic_string[1]  == 'A' &&
            magic_string[2]  == 'G' &&
            magic_string[3]  == 'I' &&
            magic_string[4]  == 'C' &&
            magic_string[5]  == '_' &&
            magic_string[6]  == 'P' &&
            magic_string[7]  == 'R' &&
            magic_string[8]  == 'E' &&
            magic_string[9]  == 'S' &&
            magic_string[10] == 'E' &&
            magic_string[11] == 'N' &&
            magic_string[12] == 'T')
            log() << "Magic test passed!";
        else
        {
            error() << "Magic test failed! magic_string=" << format::as_address << magic_string;
            hang();
        }
    }

    void init_global_objects()
    {
        for (global_constructor_t* ctor = &global_constructors_begin; ctor < &global_constructors_end; ++ctor)
            (*ctor)();
    }
}
