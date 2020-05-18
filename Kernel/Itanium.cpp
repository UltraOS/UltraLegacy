#include "Log.h"

extern "C" void __cxa_pure_virtual()
{
    kernel::log() << "A pure virtual call!\n";
    for(;;);
}