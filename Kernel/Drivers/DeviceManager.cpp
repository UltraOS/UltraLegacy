#include "DeviceManager.h"
#include "Common/Logger.h"
#include "Common/Utilities.h"

namespace kernel {

DeviceManager* DeviceManager::s_instance;

List<DeviceManager::DeviceInfo>& DeviceManager::devices_of_category(Device::Category category)
{
    auto index = static_cast<size_t>(category);
    ASSERT(index < static_cast<size_t>(Device::Category::LAST));

    return m_devices[index];
}

Atomic<Device*>& DeviceManager::primary_of_category(Device::Category category)
{
    auto index = static_cast<size_t>(category);
    ASSERT(index < static_cast<size_t>(Device::Category::LAST));

    return m_primary_devices[index];
}

DynamicArray<Device*> DeviceManager::all()
{
    LOCK_GUARD(m_lock);

    DynamicArray<Device*> devices(m_device_count);

    for (auto& category : m_devices) {
        for (auto& dev : category) {
            devices.append(dev.device);
        }
    }

    return devices;
}

DynamicArray<Device*> DeviceManager::all_of(Device::Category category, StringView type)
{
    LOCK_GUARD(m_lock);

    DynamicArray<Device*> out;

    for (auto& dev_info : devices_of_category(category)) {
        if (type == "any"_sv || dev_info.device->matches_type(type))
            out.append(dev_info.device);
    }

    return out;
}

Device* DeviceManager::one_of(Device::Category category, StringView type)
{
    LOCK_GUARD(m_lock);

    for (auto& dev_info : devices_of_category(category)) {
        if (type == "any"_sv || dev_info.device->matches_type(type))
            return dev_info.device;
    }

    return nullptr;
}

DeviceManager::DeviceInfo* DeviceManager::find_by_pointer(Device* device)
{
    auto& devices = devices_of_category(device->device_category());
    auto dev = linear_search_for(devices.begin(), devices.end(),
        [device](const DeviceInfo& dev_info) {
            return dev_info.device == device;
        });

    return dev == devices.end() ? nullptr : dev.node();
}

DynamicArray<Device*> DeviceManager::children_of(Device* device)
{
    LOCK_GUARD(m_lock);

    auto* dev = find_by_pointer(device);

    if (!dev || dev->children.empty())
        return {};

    DynamicArray<Device*> children(dev->children.size());

    for (auto child : dev->children)
        children.append(child);

    return children;
}

bool DeviceManager::has_children(Device* device)
{
    LOCK_GUARD(m_lock);

    auto* dev = find_by_pointer(device);

    return dev ? !dev->children.empty() : false;
}

void DeviceManager::register_self(Device* device)
{
    LOCK_GUARD(m_lock);

    m_device_count++;
    auto* dev_info = new DeviceInfo;
    dev_info->device = device;

    auto& devices = devices_of_category(device->device_category());
    devices.insert_back(*dev_info);

    if (devices.size() == 1)
        set_as_primary(device);
}

void DeviceManager::unregister_self(Device* device)
{
    LOCK_GUARD(m_lock);

    auto* dev = find_by_pointer(device);
    ASSERT(dev != nullptr);

    // TODO: do something with children?
    dev->pop_off();

    auto category = device->device_category();
    auto& devices = devices_of_category(category);

    // Atomically change the current primary device of this category iff it's set to current
    if (!devices.empty())
        primary_of_category(category).compare_and_exchange(&device, devices.front().device);

    delete dev;
    m_device_count--;
}

void DeviceManager::add_child(Device* parent, Device* child)
{
    LOCK_GUARD(m_lock);

    auto* dev = find_by_pointer(parent);
    ASSERT(dev != nullptr);

    dev->children.append_back(child);
}

size_t DeviceManager::count_of(Device::Category category)
{
    LOCK_GUARD(m_lock);
    return devices_of_category(category).size();
}

void DeviceManager::set_as_primary(Device* device)
{
    primary_of_category(device->device_category()).store(device, MemoryOrder::RELEASE);
}

size_t DeviceManager::count()
{
    LOCK_GUARD(m_lock);
    return m_device_count;
}

}
