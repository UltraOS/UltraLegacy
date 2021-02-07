#include "PCI.h"
#include "Access.h"

namespace kernel {

Access* PCI::s_access;
PCI PCI::s_instance;

Access& PCI::access()
{
    ASSERT(s_access != nullptr);
    return *s_access;
}

void PCI::detect_all()
{
    s_access = Access::detect();

    if (!s_access)
        return;

    m_devices = access().list_all_devices();
}

}
