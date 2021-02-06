#include "Access.h"
#include "MMIOAccess.h"
#include "ACPI/ACPI.h"

namespace kernel {

Access* Access::s_instance;

void Access::detect()
{
    if (ACPI::the().get_table_info("MCFG"_sv) == nullptr)
        FAILED_ASSERTION("This computer doesn't support PCIe, support for legacy PCI is not implemented");

    s_instance = new MMIOAccess();
}

}
