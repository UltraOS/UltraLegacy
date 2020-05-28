#include "Logger.h"
#include "Macros.h"

extern "C" void __cxa_pure_virtual()
{
    kernel::error() << "A pure virtual call!\n";
    hang();
}