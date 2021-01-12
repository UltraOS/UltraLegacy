#include "ACPI.h"
#include "Common/String.h"
#include "Memory/MemoryManager.h"

namespace kernel {

ACPI ACPI::s_instance;

void ACPI::initialize()
{
    if (find_rsdp()) {
        log() << "ACPI: found RSDP @ " << MemoryManager::virtual_to_physical(m_rsdp)
              << ". Revision " << m_rsdp->revision << ", rsdt @ " << format::as_hex << m_rsdp->rsdt_pointer;
    } else {
        log() << "ACPI: couldn't find RSDP";
    }
}

bool ACPI::find_rsdp()
{
    static constexpr StringView rsdp_signature = "RSD PTR "_sv;
    static constexpr size_t rsdp_alignment = 16;

    static constexpr Address ebda_base = 0x80000;
    auto ebda_range = Range::from_two_pointers(
            MemoryManager::physical_to_virtual(ebda_base),
            MemoryManager::physical_to_virtual(ebda_base + 1 * KB));

    auto rsdp = rsdp_signature.find_in_range(ebda_range, rsdp_alignment);

    if (!rsdp.empty()) {
        m_rsdp = Address(rsdp.data()).as_pointer<RSDP>();
        return true;
    }

    static constexpr Address bios_base = 0xE0000;
    static constexpr Address bios_end = 0xFFFFF;
    auto bios_range = Range::from_two_pointers(
            MemoryManager::physical_to_virtual(bios_base),
            MemoryManager::physical_to_virtual(bios_end));

    rsdp = rsdp_signature.find_in_range(bios_range, rsdp_alignment);

    if (!rsdp.empty()) {
        m_rsdp = Address(rsdp.data()).as_pointer<RSDP>();
        return true;
    }

    return false;
}

}