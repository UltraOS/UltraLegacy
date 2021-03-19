#pragma once

#include "AHCIPort.h"
#include "Common/Span.h"
#include "Drivers/Device.h"
#include "Drivers/PCI/PCI.h"
#include "Memory/TypedMapping.h"

namespace kernel {

class AHCIPort;
class CommandList;
class PortInterruptStatus;
class PortSATAError;
class HBA;

class AHCI final : public Device, public PCI::Device, public IRQHandler {
    AUTO_DETECT_PCI(AHCI);

public:
    explicit AHCI(const PCI::DeviceInfo&);

    static constexpr u8 class_code = 0x01;
    static constexpr u8 subclass_code = 0x06;
    static constexpr u8 programming_interface = 1;

    StringView device_type() const override { return "AHCI"_sv; }
    StringView device_model() const override { return "Generic AHCI Controller"_sv; }

    static void autodetect(const DynamicArray<PCI::DeviceInfo>&);

    struct OP {
        enum class Type {
            READ = 0,
            WRITE = 1
        } type;

        StringView type_to_string() const
        {
            switch (type) {
                case Type::READ:
                    return "READ"_sv;
                case Type::WRITE:
                    return "WRITE"_sv;
                default:
                    return "INVALID"_sv;
            }
        }

        bool is_async;
        size_t port;
        size_t command_slot;
        LBARange lba_range;

        DynamicArray<Range> physical_ranges;
    };

    struct PortState {
        // ---- immutable state (no locking needed) ----
        enum class Type : u32 {
            NONE = 0xDEADBEEF,
            SATA = 0x00000101,
            ATAPI = 0xEB140101,
            ENCLOSURE_MANAGEMENT_BRIDGE = 0xC33C0101,
            PORT_MULTIPLIER = 0x96690101
        } type = Type::NONE;

        StringView type_to_string();

        bool implemented;
        bool has_device_attached;

        TypedMapping<CommandList> command_list;

        size_t logical_sector_size;
        size_t physical_sector_size;
        u64 lba_count;
        bool supports_48bit_lba;

        StorageDevice::Info::MediumType medium_type;

        u16 ata_major;
        u16 ata_minor;

        u8 lba_offset_into_physical_sector;

        String serial_number;
        String model_string;

        // ---- mutable state (state_access_lock must be held) ----
        DynamicBitArray slot_map;

        Optional<size_t> allocate_slot();
        void deallocate_slot(size_t);

        struct QueuedRequest : StandaloneListNode<QueuedRequest> {
            StorageDevice::AsyncRequest* request;
            List<OP> queued_ops;
        };
        
        List<QueuedRequest> request_queue;
        QueuedRequest* slot_to_request[32] {};
        InterruptSafeSpinLock state_access_lock;
    };

    PortState& state_of_port(size_t index)
    {
        ASSERT(index < 32);
        return m_ports[index];
    }

    void read_synchronous(size_t port, Address into_virtual_address, LBARange);
    void write_synchronous(size_t port, Address from_virtual_address, LBARange);

    void process_async_request(size_t port, StorageDevice::AsyncRequest& request);

private:
    void perform_bios_handoff();
    void enable_ahci();
    void hba_reset();

    void initialize_ports();
    PortState::Type type_of_port(size_t index);
    bool port_has_device_attached(size_t index);
    void reset_port(size_t index);
    void initialize_port(size_t index);
    void disable_dma_engines_for_all_ports();
    void disable_dma_engines_for_port(size_t index);
    void enable_dma_engines_for_port(size_t index);
    void identify_sata_port(size_t index);
    void panic_if_port_error(PortInterruptStatus, PortSATAError);
    void wait_for_port_ready(size_t index);

    void handle_irq(RegisterState&) override;
    void handle_port_irq(size_t port);
    void enable_irq() override { ASSERT_NEVER_REACHED(); }
    void disable_irq() override { ASSERT_NEVER_REACHED(); }

    // SATA spec says "Wait up to 10 milliseconds for SStatus.DET = 3h" but we'll wait for 20
    static constexpr size_t phy_wait_timeout = 20 * Time::nanoseconds_in_millisecond;

    List<OP> build_ops(size_t port, OP::Type, Address virtual_address, LBARange, bool is_async);
    void synchronous_wait_for_command_completion(size_t port, size_t slot);
    void synchronous_execute(size_t port, List<OP>& ops);
    void execute(OP&);

    template <typename T>
    size_t offset_of_port_structure(size_t index);

    template <typename T>
    T hba_read();

    template <typename T>
    T port_read(size_t index);

    template <typename T>
    void hba_write(T reg);

    template <typename T>
    void port_write(size_t index, T reg);

    Address allocate_safe_page();

private:
    TypedMapping<volatile HBA> m_hba;

    PortState m_ports[32] { };
    u8 m_command_slots_per_port { 0 };
    bool m_supports_64bit { false };
};

}
