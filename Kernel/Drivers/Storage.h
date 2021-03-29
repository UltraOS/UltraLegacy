#pragma once

#include "Device.h"
#include "Memory/MemoryManager.h"
#include "Multitasking/Blocker.h"
#include "Multitasking/Scheduler.h"

namespace kernel {

using LBARange = BasicRange<u64>;

class StorageDevice : public Device {
public:
    StorageDevice()
        : Device(Category::STORAGE)
    {
    }

    struct Info {
        StringView drive_model;
        u64 logical_block_count;
        size_t logical_block_size;
        size_t optimal_read_size;

        enum class MediumType {
            HDD,
            SSD,
            CD,
            USB,
            REMOTE,
            RAM,

            LAST
        } medium_type;

        static StringView medium_type_to_string(MediumType medium_type)
        {
            switch (medium_type) {
            case MediumType::HDD:
                return "HDD"_sv;
            case MediumType::SSD:
                return "SSD"_sv;
            case MediumType::CD:
                return "CD"_sv;
            case MediumType::USB:
                return "USB"_sv;
            case MediumType::REMOTE:
                return "REMOTE"_sv;
            case MediumType::RAM:
                return "RAM"_sv;
            default:
                return "UNKNOWN"_sv;
            }
        }

        StringView medium_type_to_string() const
        {
            return medium_type_to_string(medium_type);
        }
    };

    class AsyncRequest {
    public:
        enum class Type {
            READ,
            WRITE
        };

        AsyncRequest(Address virtual_address, LBARange lba_range, Type type);

        static AsyncRequest make_read(Address virtual_address, LBARange lba_range)
        {
            return { virtual_address, lba_range, Type::READ };
        }

        static AsyncRequest make_write(Address virtual_address, LBARange lba_range)
        {
            return { virtual_address, lba_range, Type::WRITE };
        }

        void wait();
        void complete();

        Type type() const { return m_type; }
        Address virtual_address() const { return m_virtual_address; }
        LBARange lba_range() const { return m_lba_range; }

    private:
        DiskIOBlocker m_blocker;

        Address m_virtual_address;
        LBARange m_lba_range;
        Type m_type;

        Atomic<bool> m_is_completed { false };
    };

    virtual void submit_request(AsyncRequest&) = 0;
    virtual void read_synchronous(Address into_virtual_address, LBARange) = 0;
    virtual void write_synchronous(Address from_virtual_address, LBARange) = 0;

    virtual Info query_info() const = 0;
};

}