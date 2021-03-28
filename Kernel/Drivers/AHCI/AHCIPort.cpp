#include "AHCIPort.h"
#include "AHCI.h"

namespace kernel {

AHCIPort::AHCIPort(AHCI& parent, size_t index)
    : m_controller(parent)
    , m_index(index)
{
}

void AHCIPort::read_synchronous(Address into_virtual_address, LBARange range)
{
    m_controller.read_synchronous(m_index, into_virtual_address, range);
}

void AHCIPort::write_synchronous(Address from_virtual_address, LBARange range)
{
    m_controller.write_synchronous(m_index, from_virtual_address, range);
}

void AHCIPort::submit_request(StorageDevice::AsyncRequest& request)
{
    m_controller.process_async_request(m_index, request);
}

StringView AHCIPort::device_model() const
{
    return m_controller.state_of_port(m_index).model_string.to_view();
}

StorageDevice::Info AHCIPort::query_info() const
{
    auto& state = m_controller.state_of_port(m_index);

    Info info {};
    info.drive_model = state.model_string.to_view();
    info.logical_block_count = state.logical_block_count;
    info.logical_block_size = state.logical_sector_size;
    info.optimal_read_size = state.physical_sector_size;
    info.medium_type = state.medium_type;

    return info;
}

}