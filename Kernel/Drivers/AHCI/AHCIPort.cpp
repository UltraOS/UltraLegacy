#include "AHCIPort.h"
#include "AHCI.h"

namespace kernel {

AHCIPort::AHCIPort(AHCI& parent, size_t index)
    : m_parent(parent)
    , m_index(index)
{
}

void AHCIPort::read_synchronous(Address into_virtual_address, size_t first_lba, size_t lba_count)
{
    m_parent.read_synchronous(m_index, into_virtual_address, first_lba, lba_count);
}

void AHCIPort::write_synchronous(Address from_virtual_address, size_t first_lba, size_t lba_count)
{
    m_parent.read_synchronous(m_index, from_virtual_address, first_lba, lba_count);
}

StorageDevice::Info AHCIPort::query_info() const
{
    auto& state = m_parent.state_of_port(m_index);

    Info info {};
    info.lba_count = state.lba_count;
    info.lba_size = state.logical_sector_size;
    info.optimal_read_size = state.physical_sector_size;

    return info;
}

}