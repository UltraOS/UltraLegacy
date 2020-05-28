#include "Logger.h"

extern "C" void __cxa_pure_virtual()
{
    kernel::error() << "A pure virtual call!\n";
    for(;;);
}