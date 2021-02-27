#pragma once

#include "Common/DynamicArray.h"
#include "Common/List.h"
#include "Common/Lock.h"
#include "Device.h"

namespace kernel {

class DeviceManager {
    MAKE_SINGLETON(DeviceManager) = default;

public:
    static void initialize()
    {
        ASSERT(s_instance == nullptr);
        s_instance = new DeviceManager();
    }

    static DeviceManager& the()
    {
        ASSERT(s_instance != nullptr);
        return *s_instance;
    }

    static bool is_initialized()
    {
        return s_instance != nullptr;
    }

    DynamicArray<Device*> all();
    DynamicArray<Device*> all_of(Device::Category, StringView type = "any"_sv);
    Device* one_of(Device::Category, StringView type = "any"_sv);
    DynamicArray<Device*> children_of(Device*);
    bool has_children(Device*);
    Device* primary_of(Device::Category category) { return primary_of_category(category); }

    size_t count_of(Device::Category);
    size_t count();

    template <typename Pred>
    Device* find_if(Device::Category category, Pred predicate)
    {
        LOCK_GUARD(m_lock);

        for (auto& dev_info : devices_of_category(category)) {
            if (predicate(dev_info.device))
                return dev_info.device;
        }

        return nullptr;
    }

private:
    friend class Device;
    void register_self(Device*);
    void unregister_self(Device*);
    void add_child(Device* parent, Device* child);
    void set_as_primary(Device*);

    struct DeviceInfo : public StandaloneListNode<DeviceInfo> {
        Device* device;
        List<Device*> children;
    };

    List<DeviceInfo>& devices_of_category(Device::Category category);
    Atomic<Device*>& primary_of_category(Device::Category);

    DeviceInfo* find_by_pointer(Device*);

private:
    size_t m_device_count { 0 };
    List<DeviceInfo> m_devices[static_cast<size_t>(Device::Category::LAST)] {};
    Atomic<Device*> m_primary_devices[static_cast<size_t>(Device::Category::LAST)] {};

    mutable InterruptSafeSpinLock m_lock;

    static DeviceManager* s_instance;
};

}