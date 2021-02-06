#include "PCI.h"
#include "Access.h"

namespace kernel {

PCI PCI::s_instance;

void PCI::detect_all()
{
    Access::detect();
}

}
