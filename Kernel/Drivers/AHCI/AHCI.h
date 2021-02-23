#pragma once

#include "Drivers/PCI/PCI.h"
#include "Memory/TypedMapping.h"

namespace kernel {

class AHCI : public PCI::Device, public IRQHandler {
    AUTO_DETECT_PCI(AHCI);

public:
    AHCI(const PCI::DeviceInfo&);

    static constexpr u8 class_code = 0x01;
    static constexpr u8 subclass_code = 0x06;
    static constexpr u8 programming_interface = 1;

    Device::Type device_type() const override { return Device::Type::STORAGE; }
    StringView device_name() const override { return "AHCI"_sv; }

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

        bool contains_errors()
        {
            bool error =
                    cold_port_detect_status ||
                    task_file_error_status ||
                    host_bus_fatal_error_status ||
                    host_bus_data_error_status ||
                    interface_fatal_error_status ||
                    interface_non_fatal_error_status ||
                    overflow_status ||
                    incorrect_port_multiplier_status ||
                    phyrdy_change_status ||
                    device_mechanical_presence_status ||
                    port_connect_change_status ||
                    unknown_fis_interrupt;

            return error;
        }

        StringView error_string()
        {
            if (cold_port_detect_status)
                return "Cold presence detection status change"_sv;
            if (task_file_error_status)
                return "Task file error"_sv;
            if (host_bus_fatal_error_status)
                return "Host bus fatal error"_sv;
            if (host_bus_data_error_status)
                return "Host bus data error"_sv;
            if (interface_fatal_error_status)
                return "Interface fatal error"_sv;
            if (interface_non_fatal_error_status)
                return "Interface non-fatal error"_sv;
            if (overflow_status)
                return "Data overflow"_sv;
            if (incorrect_port_multiplier_status)
                return "Incorrect port multiplier"_sv;
            if (phyrdy_change_status)
                return "Physical layer state changed"_sv;
            if (device_mechanical_presence_status)
                return "Device mechanical presence changed"_sv;
            if (port_connect_change_status)
                return "Port connection changed status"_sv;
            if (unknown_fis_interrupt)
                return "Unknown FIS interrupt"_sv;

            return "<Unknown or no error>";
        }

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
        u8 current_command_slot : 5;
        u8 mechanical_presence_switch_state : 1;
        u8 fis_receive_running : 1;
        u8 command_list_running : 1;
        u8 cold_presence_state : 1;
        u8 port_multiplier_attached : 1;
        u8 hot_plug_capable_port : 1;
        u8 mechanical_presence_switch_attached_to_port : 1;
        u8 cold_presence_detection : 1;
        u8 external_sata_port : 1;
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
        u8 port_multiplier_port : 4;
        u32 reserved : 12;

        static constexpr size_t offset = 0x2C;
    };

    struct PACKED PortSATAError {
        enum class Error : u16 {
            RECOVERED_DATA_INTEGRITY_ERROR = SET_BIT(0),
            RECOVERED_COMMUNICATIONS_ERROR = SET_BIT(1),
            TRANSIENT_DATA_INTEGRITY_ERROR = SET_BIT(8),
            PERSISTENT_COMMUNICATION_OR_DATA_INTEGRITY_ERROR = SET_BIT(9),
            PROTOCOL_ERROR = SET_BIT(10),
            INTERNAL_ERROR = SET_BIT(11)
        } code;

        enum class Diagnostics : u16 {
            PHY_RDY_CHANGE = SET_BIT(1),
            PHY_RDY_INTERNAL_ERROR = SET_BIT(2),
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

        struct PACKED Port {
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

        static constexpr size_t offset_to_ports = 0x100;
        static constexpr size_t size = 0x1100;
    };

    static_assert(sizeof(HBA::Port) == HBA::Port::size, "Incorrect size of HBA port");
    static_assert(sizeof(HBA) == HBA::size, "Incorrect size of HBA");

    struct PACKED CommandHeader {
        u8 command_fis_length : 5;
        u8 atapi : 1;
        u8 write : 1;
        u8 prefetchable : 1;
        u8 reset : 1;
        u8 bist : 1;
        u8 clear_busy_upon_r_ok : 1;
        u8 reserved_1 : 1;
        u8 port_multiplier_port : 4;
        u32 physical_region_descriptor_table_length : 16;
        u32 physical_region_descriptor_byte_count;
        u32 command_table_base_address;
        u32 command_table_base_address_upper;
        u32 reserved_2;
        u32 reserved_3;
        u32 reserved_4;
        u32 reserved_5;

        static constexpr size_t size = 8 * sizeof(u32);
    };

    struct PACKED CommandList {
        CommandHeader commands[32];
    };

    struct PACKED PRDT {
        u32 data_base;
        u32 data_upper;

        u32 reserved_1;

        u32 byte_count : 22;
        u32 reserved_2 : 9;
        u32 interrupt_on_completion : 1;

        static constexpr size_t size = 4 * sizeof(u32);
    };

    struct PACKED CommandTable {
        u8 command_fis[64];
        u8 atapi_command[16];
        u8 reserved[48];

        PRDT prdts[];

        static constexpr size_t size = 0x80;
    };

    static_assert(sizeof(CommandHeader) == CommandHeader::size, "Incorrect size of CommandHeader");
    static_assert(sizeof(PRDT) == PRDT::size, "Incorrect size of PRDT");
    static_assert(sizeof(CommandTable) == CommandTable::size, "Incorrect size of command table");

    enum class FISType : u8 {
        REGISTER_HOST_TO_DEVICE = 0x27,
        REGISTER_DEVICE_TO_HOST = 0x34,
        DMA_ACTIVATE = 0x39,
        DMA_SETUP = 0x41,
        DATA = 0x46,
        BIST = 0x58,
        PIO_SETUP = 0x5F,
        SET_DEVICE_BITS = 0xA1,
    };

    enum class ATACommand : u8 {
        CFA_ERASE_SECTORS = 0xC0,
        CFA_REQUEST_EXTENDED_ERROR_CODE = 0x03,
        CFA_TRANSLATE_SECTOR = 0x87,
        CFA_WRITE_MULTIPLE_WITHOUT_ERASE = 0xCD,
        CFA_WRITE_SECTORS_WITHOUT_ERASE = 0x38,
        CHECK_MEDIA_CARD_TYPE = 0xD1,
        CHECK_POWER_MODE = 0xE5,
        CONFIGURE_STREAM = 0x51,
        DEVICE_CONFIGURATION = 0xB1, // feature reg = 0xC0
        DEVICE_CONFIGURATION_FREEZE_LOCK = 0xB1, // feature reg = 0xC1
        DEVICE_CONFIGURATION_IDENTITY = 0xB1, // feature reg = 0xC2
        DEVICE_CONFIGURATION_SET = 0xB1, // feature reg = 0xC3
        DEVICE_RESET = 0x08,
        DOWNLOAD_MICROCODE = 0x92,
        EXECUTE_DEVICE_DIAGNOSTICS = 0x90,
        FLUSH_CACHE = 0xE7,
        FLUSH_CACHE_EXT = 0xEA,
        GET_MEDIA_STATUS = 0xDA,
        IDENTIFY_DEVICE = 0xEC,
        IDENTIFY_PACKET_DEVICE = 0xA1,
        IDLE = 0xE3,
        IDLE_IMMEDIATE = 0xE1,
        MEDIA_EJECT = 0xED,
        MEDIA_LOCK = 0xDE,
        MEDIA_UNLOCK = 0xDF,
        NOP = 0x00,
        PACKET = 0xA0,
        READ_BUFFER = 0xE4,
        READ_DMA = 0xC8,
        READ_DMA_EXT = 0x25,
        READ_DMA_QUEUED = 0xC7,
        READ_DMA_QUEUED_EXT = 0x26,
        READ_LOG_EXT = 0x2F,
        READ_MULTIPLE = 0xC4,
        READ_MULTIPLE_EXIT = 0x29,
        READ_NATIVE_MAX_ADDRESS = 0xF8,
        READ_NATIVE_MAX_ADDRESS_EXT = 0x27,
        READ_SECTORS = 0x20,
        READ_SECTORS_EXT = 0x24,
        READ_STREAM_DMA_EXT = 0x2A,
        READ_STREAM_EXT = 0x2B,
        READ_VERIFY_SECTORS = 0x40,
        READ_VERIFY_SECTORS_EXT = 0x42,
        SECURITY_DISABLE_PASSWORD = 0xF6,
        SECURITY_ERASE_PREPARE = 0xF3,
        SECURITY_ERASE_UNIT = 0xF4,
        SECURITY_FREEZE_LOCK = 0xF5,
        SECURITY_SET_PASSWORD = 0xF1,
        SECURITY_UNLOCK = 0xF2,
        SERVICE = 0xA2,
        SET_FEATURES = 0xEF,
        SET_MAX_ADDRESS = 0xF9,
        SET_MAX_SET_PASSWORD = 0xF9, // feature reg = 0x01
        SET_MAX_LOCK = 0xF9, // feature reg = 0x02
        SET_MAX_UNLOCK = 0xF9, // feature reg = 0x03
        SET_MAX_FREEZE_LOCK = 0xF9, // feature reg = 0x04
        SET_MAX_ADDRESS_EXT = 0x37,
        SET_MULTIPLE_MODE = 0xC6,
        SLEEP = 0xE6,
        SMART_DISABLE_OPERATIONS = 0xB0, // feature reg = 0xD9
        SMART_ENABLE_DISABLE_AUTOSAVE = 0xB0, // feature reg = 0xD2
        SMART_ENABLE_OPERATIONS = 0xB0, // feature reg = 0xD8
        SMART_EXECUTE_OFFLINE_IMMEDIATE = 0xB0, // feature reg = 0xD4
        SMART_READ_DATA = 0xB0, // feature reg = 0xD0
        SMART_READ_LOG = 0xB0, // feature reg = 0xD5
        SMART_RETURN_STATUS = 0xB0, // feature reg = 0xDA
        SMART_WRITE_LOG = 0xB0, // feature reg = 0xD6
        STANDBY = 0xE2,
        STANDBY_IMMEDIATE = 0xE0,
        WRITE_BUFFER = 0xE8,
        WRITE_DMA = 0xCA,
        WRITE_DMA_EXT = 0x35,
        WRITE_DMA_FUA_EXT = 0x3D,
        WRITE_DMA_QUEUED = 0xCC,
        WRITE_DMA_QUEUED_EXIT = 0x36,
        WRITE_DMA_QUEUED_FUA_EXT = 0x3E,
        WRITE_LOG_EXT = 0x3F,
        WRITE_MULTIPLE = 0xC5,
        WRITE_MULTIPLE_EXT = 0x39,
        WRITE_MULTIPLE_FUA_EXT = 0xCE,
        WRITE_SECTORS = 0x30,
        WRITE_SECTORS_EXT = 0x34,
        WRITE_STREAM_DMA_EXT = 0x3A,
        WRITE_STREAM_EXT = 0x3B,
    };

    struct PACKED FISHostToDevice {
        FISType type = native_type;

        u8 port_multiplier : 4;
        u8 reserved_1 : 3;
        u8 is_command : 1;

        ATACommand command_register;
        u8 feature_lower;

        u8 lba_1;
        u8 lba_2;
        u8 lba_3;

        u8 device_register;

        u8 lba_4;
        u8 lba_5;
        u8 lba_6;

        u8 feature_upper;

        u8 count_lower;
        u8 count_upper;

        u8 icc;
        u8 control_register;

        u32 reserved_2;

        static constexpr FISType native_type = FISType::REGISTER_HOST_TO_DEVICE;
        static constexpr size_t size_in_dwords = 5;
    };

    struct PACKED FISDeviceToHost {
        FISType type = native_type;

        u8 port_multiplier : 4;
        u8 reserved_1 : 2;
        u8 interrupt : 1;
        u8 reserved_2 : 1;

        u8 status_register;
        u8 error_register;

        u8 lba_1;
        u8 lba_2;
        u8 lba_3;

        u8 device_register;

        u8 lba_4;
        u8 lba_5;
        u8 lba_6;

        u8 reserved_3;

        u8 count_lower;
        u8 count_upper;

        u8 reserved_4[6];

        static constexpr FISType native_type = FISType::REGISTER_DEVICE_TO_HOST;
        static constexpr size_t size_in_dwords = 5;
    };

    struct PACKED FISData {
        FISType type = native_type;

        u8 port_multiplier : 4;
        u8 reserved_1 : 4;
        u8 reserved_2[2];

        u32 data[1];

        static constexpr FISType native_type = FISType::DATA;
    };

    struct PACKED FISPIOSetup {
        FISType type = native_type;

        u8 port_multiplier : 4;
        u8 reserved_1 : 1;
        u8 data_transfer_direction : 1;
        u8 interrupt : 1;
        u8 reserved_2 : 1;

        u8 status_register;
        u8 error_register;

        u8 lba_1;
        u8 lba_2;
        u8 lba_3;

        u8 device_register;

        u8 lba_4;
        u8 lba_5;
        u8 lba_6;

        u8 reserved_3;

        u8 count_lower;
        u8 count_upper;

        u8 reserved_4;
        u8 new_status_register;

        u16 trasnfer_count;
        u8 reserved_5[2];

        static constexpr FISType native_type = FISType::PIO_SETUP;
        static constexpr size_t size_in_dwords = 5;
    };

    struct PACKED FISDMASetup {
        FISType type = native_type;

        u8 port_multiplier : 4;
        u8 reserved_1 : 1;
        u8 data_transfer_direction : 1;
        u8 interrupt : 1;
        u8 auto_activate : 1;

        u8 reserved_2[2];

        u32 dma_buffer_id_lower;
        u32 dma_buffer_id_upper;

        u32 reserved_3;

        u32 dma_buffer_offset;
        u32 transfer_count;

        u32 reserved_4;

        static constexpr FISType native_type = FISType::DMA_SETUP;
        static constexpr size_t size_in_dwords = 7;
    };

    struct PACKED FISDeviceBits {
        FISType type = native_type;

        u8 port_multiplier : 4;
        u8 reserved_1 : 2;
        u8 interrupt : 1;
        u8 notification : 1;

        u8 status_register;
        u8 error_register;

        u32 protocol_specific;

        static constexpr FISType native_type = FISType::SET_DEVICE_BITS;
        static constexpr size_t size_in_dwords = 2;
    };

    static_assert(sizeof(FISHostToDevice) == FISHostToDevice::size_in_dwords * sizeof(u32), "Incorrect size of FISHostToDevice");
    static_assert(sizeof(FISDeviceToHost) == FISDeviceToHost::size_in_dwords * sizeof(u32), "Incorrect size of FISDeviceToHost");
    static_assert(sizeof(FISPIOSetup) == FISPIOSetup::size_in_dwords * sizeof(u32), "Incorrect size of FISPIOSetup");
    static_assert(sizeof(FISDMASetup) == FISDMASetup::size_in_dwords * sizeof(u32), "Incorrect size of FISDMASetup");
    static_assert(sizeof(FISDeviceBits) == FISDeviceBits::size_in_dwords * sizeof(u32), "Incorrect size of FISDeviceBits");

    struct PACKED HBAFIS {
        FISDMASetup dma_setup;
        u8 padding_1[4];

        FISPIOSetup pio_setup;
        u8 padding_2[12];

        FISDeviceToHost device_to_host;
        u8 padding_3[4];

        FISDeviceBits device_bits;

        u8 unknown[64];

        u8 reserved[0x100 - 0xA0];

        static constexpr size_t size = 0x100;
    };

    static_assert(sizeof(HBAFIS) == HBAFIS::size, "Incorrect size of HBAFIS");

    struct Port {
        enum class Type : u32 {
            NONE = 0xDEADBEEF,
            SATA = 0x00000101,
            ATAPI = 0xEB140101,
            ENCLOSURE_MANAGEMENT_BRIDGE = 0xC33C0101,
            PORT_MULTIPLIER = 0x96690101
        } type = Type::NONE;
        
        StringView type_to_string()
        {
            switch (type) {
            case Type::NONE:
                return "none"_sv;
            case Type::SATA:
                return "SATA"_sv;
            case Type::ATAPI:
                return "ATAPI"_sv;
            case Type::ENCLOSURE_MANAGEMENT_BRIDGE:
                return "Enclosure Management Bridge"_sv;
            case Type::PORT_MULTIPLIER:
                return "Port Multiplier"_sv;
            default:
                return "Invalid"_sv;
            }
        }

        bool implemented;
        bool has_device_attached;

        TypedMapping<CommandList> command_list;

        DynamicBitArray slot_map;

        size_t logical_sector_size;
        size_t physical_sector_size;
        u64 lba_count;
        bool supports_48bit_lba;

        u16 ata_major;
        u16 ata_minor;

        u8 lba_offset_into_physical_sector;

        String serial_number;
        String model_string;

        Optional<size_t> allocate_slot()
        {
            auto slot = slot_map.find_bit(false);

            if (slot.has_value())
                slot_map.set_bit(slot.value(), true);

            return slot;
        }
    };

private:
    void perform_bios_handoff();
    void enable_ahci();
    void hba_reset();
    void initialize_ports();
    Port::Type type_of_port(size_t index);
    bool port_has_device_attached(size_t index);
    void reset_port(size_t index);
    void initialize_port(size_t index);
    void disable_dma_engines_for_all_ports();
    void disable_dma_engines_for_port(size_t index);
    void enable_dma_engines_for_port(size_t index);
    void identify_sata_port(size_t index);

    void synchronous_complete_command(size_t port, size_t slot);

    void panic_if_port_error(PortInterruptStatus);
    
    void handle_irq(RegisterState&) override;
    void enable_irq() override { ASSERT_NEVER_REACHED(); }
    void disable_irq() override { ASSERT_NEVER_REACHED(); }

    template <typename T>
    size_t offset_of_port_structure(size_t index)
    {
        Address base = m_hba.get();
        base += HBA::offset_to_ports;
        base += HBA::Port::size * index;
        base += T::offset;

        return base;
    }

    template <typename T>
    T hba_read()
    {
        Address base = m_hba.get();
        base += T::offset;

        auto raw = *base.as_pointer<volatile u32>();

        return bit_cast<T>(raw);
    }

    template <typename T>
    T port_read(size_t index)
    {
        Address base = offset_of_port_structure<T>(index);

        auto raw = *base.as_pointer<volatile u32>();

        return bit_cast<T>(raw);
    }

    template <typename T>
    void hba_write(T reg)
    {
        Address base = m_hba.get();
        base += T::offset;

        auto type_punned_reg = bit_cast<u32>(reg);
        *base.as_pointer<volatile u32>() = type_punned_reg;
    }

    template <typename T>
    void port_write(size_t index, T reg)
    {
        Address base = offset_of_port_structure<T>(index);

        auto type_punned_reg = bit_cast<u32>(reg);
        *base.as_pointer<volatile u32>() = type_punned_reg;
    }

    Address allocate_safe_page();

private:
    TypedMapping<volatile HBA> m_hba;

    Port m_ports[32] {};
    u8 m_command_slots_per_port { 0 };
    bool m_supports_64bit { false };
};

}
