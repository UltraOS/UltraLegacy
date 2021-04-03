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

    class Request {
    public:
        enum class OP {
            READ,
            WRITE
        };

        enum class Type {
            ASYNC,
            RAMDISK
        };

        Request(Address virtual_address, OP op, Type type);

        [[nodiscard]] OP op() const { return m_op; }
        [[nodiscard]] Type type() const { return m_type; }
        [[nodiscard]] Address virtual_address() const { return m_virtual_address; }

        ~Request() = default;

    private:
        Address m_virtual_address;
        OP m_op;
        Type m_type;
    };

    class AsyncRequest : public Request {
    public:
        AsyncRequest(Address virtual_address, LBARange lba_range, OP op);

        static AsyncRequest make_read(Address virtual_address, LBARange lba_range)
        {
            return { virtual_address, lba_range, OP::READ };
        }

        static AsyncRequest make_write(Address virtual_address, LBARange lba_range)
        {
            return { virtual_address, lba_range, OP::WRITE };
        }

        void wait();
        void complete();

        [[nodiscard]] LBARange lba_range() const { return m_lba_range; }

    private:
        DiskIOBlocker m_blocker;
        LBARange m_lba_range;
        Atomic<bool> m_is_completed { false };
    };

    class RamdiskRequest : public Request {
    public:
        RamdiskRequest(Address virtual_address, size_t byte_offset, size_t byte_count, OP op);

        static RamdiskRequest make_read(Address virtual_address, size_t byte_offset, size_t byte_count)
        {
            return { virtual_address, byte_offset, byte_count, OP::READ };
        }

        static RamdiskRequest make_write(Address virtual_address, size_t byte_offset, size_t byte_count)
        {
            return { virtual_address, byte_offset, byte_count, OP::WRITE };
        }

        [[nodiscard]] size_t offset() const { return m_offset; }
        [[nodiscard]] size_t byte_count() const { return m_byte_count; }

    private:
        size_t m_offset { 0 };
        size_t m_byte_count { 0 };
    };

    virtual void submit_request(Request&) = 0;
    virtual void read_synchronous(Address into_virtual_address, LBARange) = 0;
    virtual void write_synchronous(Address from_virtual_address, LBARange) = 0;

    virtual Info query_info() const = 0;
};

}