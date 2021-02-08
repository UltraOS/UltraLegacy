#include "Access.h"
#include "ACPI/ACPI.h"
#include "MMIOAccess.h"

namespace kernel {

Access* Access::detect()
{
    if (ACPI::the().get_table_info("MCFG"_sv) == nullptr) {
        warning() << "This computer doesn't support PCIe, support for legacy PCI is not implemented";
        return nullptr;
    }

    return new MMIOAccess();
}

}
