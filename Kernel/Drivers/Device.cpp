#include "Device.h"
#include "DeviceManager.h"

namespace kernel {

Device::Device(Category category)
    : m_category(category)
{
    DeviceManager::the().register_self(this);
}

Device::~Device()
{
    DeviceManager::the().unregister_self(this);
}

void Device::make_child_of(Device* parent)
{
    DeviceManager::the().add_child(parent, this);
}

bool Device::is_primary()
{
    return DeviceManager::the().primary_of_category(m_category).load(MemoryOrder::ACQUIRE) == this;
}

void Device::make_primary()
{
    DeviceManager::the().set_as_primary(this);
}

}
