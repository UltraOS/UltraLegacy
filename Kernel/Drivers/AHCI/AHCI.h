#pragma once

#include "Drivers/PCI/PCI.h"
#include "Memory/TypedMapping.h"

namespace kernel {

class AHCI : public PCI::Device {
    AUTO_DETECT_PCI(AHCI);

public:
    AHCI(PCI::Location);

    static constexpr u8 class_code = 0x01;
    static constexpr u8 subclass_code = 0x06;
    static constexpr u8 programming_interface = 1;

    Type type() const override { return Type::DISK; }
    StringView name() const override { return "AHCI"_sv; }

    static void autodetect(const DynamicArray<PCI::DeviceInfo>&);

    struct PACKED CAP {
        u8 number_of_ports : 5;
        u8 supports_extrnal_sata : 1;
        u8 enclosure_management_supported : 1;
        u8 command_completion_coalescing_supported : 1;
        u8 number_of_command_slots : 5;
        u8 partial_state_capable : 1;
        u8 slumber_state_capable : 1;
        u8 pio_multiple_drq_block : 1;
        u8 fis_based_switching_supported : 1;
        u8 supports_port_multiplier : 1;
        u8 supports_ahci_mode_only : 1;
        u8 reserved : 1;

        enum class InterfaceSpeedSupport : u8 {
            GEN1 = SET_BIT(1),
            GEN2 = SET_BIT(2),
            GEN3 = SET_BIT(1) | SET_BIT(2),
        } interface_speed_support : 4;

        u8 supports_command_list_override : 1;
        u8 supports_activity_led : 1;
        u8 supports_aggressive_link_power_managment : 1;
        u8 supports_staggered_spin_up : 1;
        u8 supports_mechanical_presence_switch : 1;
        u8 supports_snotification_register : 1;
        u8 supports_native_command_queueing : 1;
        u8 supports_64bit_addressing : 1;

        static constexpr size_t offset = 0x0;
    };

    struct PACKED GHC {
        u8 hba_reset : 1;
        u8 interrupt_enable : 1;
        u8 msi_revert_to_single_message : 1;
        u32 reserved : 28;
        u8 ahci_enable : 1;

        static constexpr size_t offset = 0x4;
    };

    struct PACKED AHCIVersion {
        u16 minor;
        u16 major;

        static constexpr size_t offset = 0x10;
    };

    struct PACKED CommandCompletionCoalescingControl {
        u8 enable : 1;
        u8 reserved : 2;
        u8 interrupt : 5;
        u8 command_completions;
        u16 timeout_value;

        static constexpr size_t offset = 0x14;
    };

    struct PACKED EnclosureManagementLocation {
        u16 buffer_size;
        u16 buffer_offset;

        static constexpr size_t offset = 0x1C;
    };

    struct PACKED EnclosureManagementControl {
        u8 message_received : 1;
        u8 reserved_1 : 7;
        u8 transmit_message : 1;
        u8 reset : 1;
        u8 reserved_2 : 6;
        u8 led_message_types : 1;
        u8 saf_te_enclosure_management_messages : 1;
        u8 ses_2_enclosure_management_messages : 1;
        u8 sgpio_enclosure_management_messages : 1;
        u8 reserved_3 : 4;
        u8 single_message_buffer : 1;
        u8 transmit_only : 1;
        u8 activity_led_hardware_driven : 1;
        u8 port_multiplier_support : 1;
        u8 reserved_4 : 4;

        static constexpr size_t offset = 0x20;
    };

    struct CAP2 {
        u8 bios_handoff : 1;
        u8 nvmhci_present : 1;
        u8 automatic_partial_to_slumber_transitions : 1;
        u8 supports_device_sleep : 1;
        u8 supports_aggressive_device_sleep_management : 1;
        u8 devsleep_entrance_from_slumber_only : 1;
        u32 reserved : 26;

        static constexpr size_t offset = 0x24;
    };

    struct BIOSHandoffControlAndStatus {
        u32 bios_owned_semaphore : 1;
        u32 os_owned_semaphore : 1;
        u32 smi_on_os_ownership_change_enable : 1;
        u32 os_ownership_change : 1;
        u32 bios_busy : 1;
        u32 reserved : 27;

        static constexpr size_t offset = 0x28;
    };

    // ---- Ports start here ----

    struct PACKED PortInterruptStatus {
        u8 device_to_host_register_fis_interrupt : 1;
        u8 pio_setup_fis_interrupt : 1;
        u8 dma_setup_fis_interrupt : 1;
        u8 set_device_bits_interrupt : 1;
        u8 unknown_fis_interrupt : 1;
        u8 descriptor_processed : 1;
        u8 port_connect_change_status : 1;
        u8 device_mechanical_presence_status : 1;
        u32 reserved_1 : 14;
        u8 phyrdy_change_status : 1;
        u8 incorrect_port_multiplier_status : 1;
        u8 overflow_status : 1;
        u8 reserved_2 : 1;
        u8 interface_non_fatal_error_status : 1;
        u8 interface_fatal_error_status : 1;
        u8 host_bus_data_error_status : 1;
        u8 host_bus_fatal_error_status : 1;
        u8 task_file_error_status : 1;
        u8 cold_port_detect_status : 1;

        static constexpr size_t offset = 0x10;
    };

    struct PACKED PortInterruptEnable {
        u8 device_to_host_register_fis_interrupt_enable : 1;
        u8 pio_setup_fis_interrupt_enable : 1;
        u8 dma_setup_fis_interrupt_enable : 1;
        u8 set_device_bits_fis_interrupt_enable : 1;
        u8 unknown_fis_interrupt_enable : 1;
        u8 descriptor_processes_interrupt_enable : 1;
        u8 port_change_interrupt_enable : 1;
        u8 device_mechanical_presence_enable : 1;
        u32 reserved_1 : 14;
        u8 phyrdy_change_interrupt_enable : 1;
        u8 incorrect_port_multiplier_enable : 1;
        u8 overflow_enable : 1;
        u8 reserved : 1;
        u8 interface_non_fatal_error_enable : 1;
        u8 interface_fatal_error_enable : 1;
        u8 host_bus_data_error_enable : 1;
        u8 host_bus_fatal_error_enable : 1;
        u8 task_file_error_enable : 1;
        u8 cold_presence_detect_enable : 1;

        static constexpr size_t offset = 0x14;
    };

    struct PACKED PortCommandAndStatus {
        u8 start : 1;
        u8 spinup_device : 1;
        u8 power_on_device : 1;
        u8 command_list_override : 1;
        u8 fis_receive_enable : 1;
        u8 reserved : 3;
        u8 current_command_slot : 1;
        u8 mechanical_presence_switch_state : 1;
        u8 fis_receive_running : 1;
        u8 command_list_running : 1;
        u8 cold_presence_state : 1;
        u8 port_multiplier_attached : 1;
        u8 hot_plug_capable_port : 1;
        u8 mechanical_presence_swtich_attached_to_port : 1;
        u8 cold_presence_detection : 1;
        u8 external_stata_port : 1;
        u8 fis_base_switching_capable_port : 1;
        u8 automatic_partial_to_slumber_transitions_enabled : 1;
        u8 device_is_atapi : 1;
        u8 drive_led_on_atapi_enable : 1;
        u8 aggressive_link_power_management_enable : 1;
        u8 aggressive_slumber_partial : 1;

        enum class InterfaceCommunicationControl : u8 {
            NO_OP = 0,
            ACTIVE = 1,
            PARTIAL = 2,
            SLUMBER = 6,
            DEV_SLEEP = 8
        } communication_control : 4;

        static constexpr size_t offset = 0x18;
    };

    struct PACKED PortTaskFileData {
        struct PACKED status {
            u8 error : 1;
            u8 comand_specific_1 : 3;
            u8 drq : 1;
            u8 command_specific_2 : 2;
            u8 busy : 1;
        } status;

        u8 error;
        u8 reserved;

        static constexpr size_t offset = 0x20;
    };

    struct PACKED PortSignature {
        u8 sector_count;
        u8 lba_low;
        u8 lba_mid;
        u8 lbi_high;

        static constexpr size_t offset = 0x24;
    };

    struct PACKED PortSATAStatus {
        enum class DeviceDetection : u8 {
            NO_DEVICE = 0,
            DEVICE_PRESENT_NO_PHY = 1,
            DEVICE_PRESENT_PHY = 3,
            PHY_OFFLINE = 4
        } device_detection : 4;

        enum class CurrentInterfaceSpeed : u8 {
            DEVICE_NOT_PRESENT = 0,
            GEN1 = 1,
            GEN2 = 2,
            GEN3 = 3,
        } current_interface_speed : 4;

        enum class InterfacePowerManagement : u8 {
            DEVICE_NOT_PRESENT = 0,
            INTERFACE_ACTIVE = 1,
            INTERFACE_PARTIAL = 2,
            INTERFACE_SLUMBER = 6,
            INTERFACE_DEV_SLEEP = 8
        } power_management : 4;

        u32 reserved : 20;

        static constexpr size_t offset = 0x28;
    };

    struct PACKED PortSATAControl {
        enum class DeviceDetectionInitialization : u8 {
            NOT_REQUESTED = 0,
            PERFORM_INITIALIZATION = 1,
            DISABLE_SATA_AND_PUT_PHY_OFFLINE = 4
        } device_detection_initialization : 4;

        enum class SpeedAllowed : u8 {
            NO_RESTRICTIONS = 0,
            GEN1 = 1,
            GEN2 = 2,
            GEN3 = 3
        } speed_allowed : 4;

        enum class InterfacePowerManagementTransitionsAllowed : u8 {
            NO_RESTRICTIONS = 0,
            TO_PARTIAL_DISABLED = 1,
            TO_SLUMBER_DISABLED = 2,
            TO_PARTIAL_AND_SLUMBER_DISABLED = 3,
            TO_DEV_SLEEP_DISABLED = 4,
            TO_PARTIAL_AND_DEV_SLEEP_DISABLED = 5,
            TO_SLUMBER_AND_DEV_SLEEP_DISABLED = 6,
            TO_PARTIAL_SLUMBER_AND_DEV_SLEEP_DISABLED = 7
        } transitions_allowed : 4;

        u8 select_power_management : 4;
        u8 port_multipler_port : 4;
        u32 reserved : 12;

        static constexpr size_t offset = 0x2C;
    };

    struct PACKED PortSATAError {
        enum class Error : u16 {
            RECOVERED_DATA_INTEGRITY_ERROR = SET_BIT(0),
            RECOVERED_COMMUNICATIONS_ERROR = SET_BIT(1),
            TRANSIENT_DATA_INTEGRITY_ERROR = SET_BIT(8),
            PERSISTENT_COMMUNICATION_OR_DATA_INTERGRITY_ERROR = SET_BIT(9),
            PROTOCOL_ERROR = SET_BIT(10),
            INTERNAL_ERROR = SET_BIT(11)
        } code;

        enum class Diagnostics : u16 {
            PHY_RDY_CHANGE = SET_BIT(1),
            PHY_RDY_INTERNAL_ERORR = SET_BIT(2),
            COMM_WAKE = SET_BIT(3),
            DECODE_ERROR = SET_BIT(4),
            DISPARITY_ERROR = SET_BIT(5),
            CRC_ERROR = SET_BIT(6),
            HANDSHAKE_ERROR = SET_BIT(7),
            LINK_SEQUENCE_ERROR = SET_BIT(8),
            TRANSPORT_STATE_TRANSITION_ERROR = SET_BIT(9),
            UNKNOWN_FIS_TYPE = SET_BIT(10),
            EXCHANGED = SET_BIT(11)
        } diagnostics;

        static constexpr size_t offset = 0x30;
    };

    struct PACKED PortSwitchingControl {
        u8 enable : 1;
        u8 device_error_clear : 1;
        u8 single_device_error : 1;
        u8 reserved_1 : 5;
        u8 device_to_issue : 4;
        u8 active_device_optimization : 4;
        u8 device_with_error : 4;
        u32 reserved_2 : 12;

        static constexpr size_t offset = 0x40;
    };

    struct PACKED PortDeviceSleep {
        u32 aggressive_device_sleep_enable : 1;
        u32 present : 1;
        u32 sleep_exit_timeout : 8;
        u32 minimum_device_sleep_assertion_time : 5;
        u32 device_sleep_idle_timeout : 10;
        u32 dito_multiplier : 4;
        u32 reserved : 3;

        static constexpr size_t offset = 0x44;
    };

    struct PACKED HBA {
        u32 capabilities;
        u32 ghc;
        u32 interrupt_pending_status;
        u32 ports_implemented;
        u32 ahci_version;
        u32 command_completion_coalescing_control;
        u32 command_completion_coalescing_ports;
        u32 enclosure_management_location;
        u32 enclosure_management_control;
        u32 capabilities_extended;
        u32 bios_handoff;

        u8 reserved[0x60 - 0x2C];
        u8 nvmhci_reserved[0xA0 - 0x60];
        u8 vendor_specific_registers[0x100 - 0xA0];

        struct Port {
            u32 command_list_base_address;
            u32 command_list_base_address_upper;
            u32 fis_base_address;
            u32 fis_base_address_upper;
            u32 interrupt_status;
            u32 interrupt_enable;
            u32 command_and_status;
            u32 reserved_1;
            u32 task_file_data;
            u32 signature;
            u32 sata_status;
            u32 stata_control;
            u32 error;
            u32 sata_active;
            u32 command_issue;
            u16 pm_notify;
            u16 reserved_2;
            u32 switching_control;
            u32 device_sleep;
            u8 reserved_3[0x70 - 0x48];
            u8 vendor_specific[0x80 - 0x70];

            static constexpr size_t size = 0x80;
        } ports[32];

        static constexpr size_t size = 0x1100;
    };

    static_assert(sizeof(HBA::Port) == HBA::Port::size, "Incorrect size of HBA port");
    static_assert(sizeof(HBA) == HBA::size, "Incorrect size of HBA");

private:
    void perform_bios_handoff();
    void enable_ahci();
    void hba_reset();

    template <typename T>
    T hba_read()
    {
        Address base = m_hba.get();
        base += T::offset;

        auto raw = *base.as_pointer<volatile u32>();

        return bit_cast<T>(raw);
    }

    template <typename T>
    void hba_write(T reg)
    {
        Address base = m_hba.get();
        auto type_punned_reg = bit_cast<u32>(reg);
        *base.as_pointer<volatile u32>() = type_punned_reg;
    }

private:
    TypedMapping<volatile HBA> m_hba;
    bool m_supports_64bit { false };
};

}
