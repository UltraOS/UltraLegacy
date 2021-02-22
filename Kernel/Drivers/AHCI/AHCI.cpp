#include "AHCI.h"
#include "Common/Logger.h"
#include "Drivers/PCI/Access.h"
#include "Interrupts/Timer.h"
#include "Multitasking/Sleep.h"

#ifdef ULTRA_32

#define SET_DWORDS_TO_ADDRESS(lower, upper, address) lower = address & 0xFFFFFFFF
#define ADDRESS_FROM_TWO_DWORDS(lower, upper) Address(lower)

#elif defined(ULTRA_64)

#define SET_DWORDS_TO_ADDRESS(lower, upper, address) lower = address & 0xFFFFFFFF; upper = address >> 32
#define ADDRESS_FROM_TWO_DWORDS(lower, upper) Address(lower | (static_cast<Address::underlying_pointer_type>(upper) << 32))

#endif

namespace kernel {

AHCI::AHCI(const PCI::DeviceInfo& info)
    : Device(info), IRQHandler(IRQHandler::Type::MSI)
{
    enable_memory_space();
    make_bus_master();
    clear_interrupts_disabled();

    auto bar_5 = bar(5);
    ASSERT(bar_5.type == BAR::Type::MEMORY);
    m_hba = TypedMapping<volatile HBA>::create("AHCI HBA"_sv, bar_5.address);

    // We have to enable AHCI before touching any AHCI registers
    enable_ahci();

    perform_bios_handoff();
    disable_dma_engines_for_all_ports();
    hba_reset();

    auto ghc = hba_read<GHC>();
    ghc.interrupt_enable = true;
    hba_write(ghc);

    log("AHCI") << "enabling MSI for interrupt vector " << interrupt_vector();
    enable_msi(interrupt_vector());

    auto cap = hba_read<CAP>();
    m_supports_64bit = cap.supports_64bit_addressing;
    m_command_slots_per_port = cap.number_of_command_slots + 1;

    log("AHCI") << "detected " << m_command_slots_per_port << " command slots per port";

    initialize_ports();
}

void AHCI::disable_dma_engines_for_all_ports()
{
    auto implemented_ports = m_hba->ports_implemented;

    for (size_t i = 0; i < 32; ++i) {
        if (!(implemented_ports & SET_BIT(i)))
            continue;

        disable_dma_engines_for_port(i);
    }
}

void AHCI::initialize_ports()
{
    auto implemented_ports = m_hba->ports_implemented;

    log("AHCI") << "detected " << __builtin_popcount(implemented_ports) << " implemented ports";

    for (size_t i = 0; i < 32; ++i) {
        if (!(implemented_ports & SET_BIT(i)))
            continue;

        m_ports[i].implemented = true;
        initialize_port(i);
    }
}

bool AHCI::port_has_device_attached(size_t index) {
    auto status = port_read<PortSATAStatus>(index);

    return status.power_management == PortSATAStatus::InterfacePowerManagement::INTERFACE_ACTIVE &&
           status.device_detection == PortSATAStatus::DeviceDetection::DEVICE_PRESENT_PHY;
}

AHCI::Port::Type AHCI::type_of_port(size_t index)
{
    auto& port = m_hba->ports[index];

    switch (static_cast<Port::Type>(port.signature)) {
    case Port::Type::SATA:
    case Port::Type::ATAPI:
    case Port::Type::ENCLOSURE_MANAGEMENT_BRIDGE:
    case Port::Type::PORT_MULTIPLIER:
        return static_cast<Port::Type>(port.signature);
    default:
        return Port::Type::NONE;
    }
}

void AHCI::initialize_port(size_t index)
{
    if (!port_has_device_attached(index))
        return;

    disable_dma_engines_for_port(index);

    auto& port = m_ports[index];
    port.has_device_attached = true;

    auto command_list_page = allocate_safe_page();
    auto base_fis_page = allocate_safe_page();

    auto& this_port = m_hba->ports[index];

    SET_DWORDS_TO_ADDRESS(this_port.command_list_base_address, this_port.command_list_base_address_upper, command_list_page);
    SET_DWORDS_TO_ADDRESS(this_port.fis_base_address, this_port.fis_base_address_upper, base_fis_page);

    port.command_list = TypedMapping<CommandList>::create("AHCI Command List"_sv, command_list_page);

    for (size_t i = 0; i < m_command_slots_per_port; ++i) {
        auto& command = port.command_list->commands[i];

        auto page = allocate_safe_page();
        SET_DWORDS_TO_ADDRESS(command.command_table_base_address, command.command_table_base_address_upper, page);
    }

    // NOTE: We have to reset *after* we set both fis & command list registers,
    //       otherwise the port will not be able to send an init D2H to retrieve
    //       the signature & other info
    // NOTE 2: Not resetting the port after resetting the HBA causes the port to hang
    //         forever for whatever reason on real hardware
    reset_port(index);
    
    enable_dma_engines_for_port(index);

    port.type = type_of_port(index);

    if (port.type == Port::Type::NONE) {
        log("AHCI") << "port " << index << " doesn't have a device attached";
        return;
    } else if (port.type != Port::Type::SATA) {
        log("AHCI") << "port " << index << " has type \"" << port.type_to_string() << "\", which is currently not supported";
        return;
    } else {
        log("AHCI") << "detected device of type \"" << port.type_to_string() << "\" on port " << index;
    }

    identify_sata_port(index);

    // Clear all previously issued interrupts
    m_hba->ports[index].interrupt_status = 0xFFFFFFFF;

    // Enable all interrupts
    m_hba->ports[index].interrupt_enable = 0xFFFFFFFF;

    log() << "AHCI: successfully intitialized port " << index;
}

void AHCI::disable_dma_engines_for_port(size_t index)
{
    auto cmd = port_read<PortCommandAndStatus>(index);

    if (!cmd.start && !cmd.command_list_running && !cmd.fis_receive_enable && !cmd.fis_receive_running) {
        log("AHCI") << "DMA engines for port " << index << " are already off";
        return;
    }

    cmd.start = false;
    cmd.fis_receive_enable = false;
    port_write(index, cmd);

    static constexpr size_t port_wait_timeout = 500 * Time::nanoseconds_in_millisecond;
    auto wait_start_ts = Timer::nanoseconds_since_boot();
    auto wait_end_ts = wait_start_ts + port_wait_timeout;

    do {
        cmd = port_read<PortCommandAndStatus>(index);
    } while ((cmd.command_list_running || cmd.fis_receive_running) && Timer::nanoseconds_since_boot() < wait_end_ts);

    if (cmd.command_list_running || cmd.fis_receive_running)
        runtime::panic("AHCI: a port failed to enter idle state in 500ms");

    log("AHCI") << "successfully disabled DMA engines for port " << index;
}

void AHCI::enable_dma_engines_for_port(size_t index)
{
    auto cmd = port_read<PortCommandAndStatus>(index);
    cmd.fis_receive_enable = true;
    port_write(index, cmd);

    static constexpr size_t port_wait_timeout = 500 * Time::nanoseconds_in_millisecond;
    auto wait_start_ts = Timer::nanoseconds_since_boot();
    auto wait_end_ts = wait_start_ts + port_wait_timeout;

    do {
        cmd = port_read<PortCommandAndStatus>(index);
    } while (cmd.command_list_running && wait_end_ts > Timer::nanoseconds_since_boot());

    if (cmd.command_list_running)
        runtime::panic("AHCI: PxCMD.CR failed to enter idle state after 500ms");

    cmd.start = true;
    port_write(index, cmd);
}

void AHCI::handle_irq(RegisterState&)
{
    volatile auto port_status = m_hba->interrupt_pending_status;
    m_hba->interrupt_pending_status = port_status;

    for (size_t i = 0; i < 32; ++i) {
        if (!IS_BIT_SET(port_status, i))
            continue;

        auto port_status = port_read<PortInterruptStatus>(i);
        port_write(i, port_status);

        panic_if_port_error(port_status);

        u32 x;
        copy_memory(&port_status, &x, 4);

        log("AHCI") << "got an irq for port " << i << " status=" << x;
    }
}

void AHCI::identify_sata_port(size_t index)
{
    static constexpr u8 command_slot_for_identify = 0;
    static constexpr u32 identify_command_buffer_size = 512;

    auto& port = m_ports[index];
    auto& command_header_0 = port.command_list->commands[command_slot_for_identify];
    Address identify_base = allocate_safe_page();

    Address command_table_0_phys = ADDRESS_FROM_TWO_DWORDS(command_header_0.command_table_base_address, command_header_0.command_table_base_address_upper);
    auto command_table_0 = TypedMapping<CommandTable>::create("AHCI command table"_sv, command_table_0_phys, sizeof(CommandTable) + sizeof(PRDT));

    command_header_0.physical_region_descriptor_table_length = 1;
    command_header_0.command_fis_length = FISHostToDevice::size_in_dwords;

    FISHostToDevice fis {};
    fis.command_register = ATACommand::IDENTIFY_DEVICE;
    fis.is_command = true;
    copy_memory(&fis, command_table_0->command_fis, sizeof(fis));

    auto& prdt_0 = command_table_0->prdts[0];
    prdt_0.byte_count = identify_command_buffer_size - 1;

    SET_DWORDS_TO_ADDRESS(prdt_0.data_base, prdt_0.data_upper, identify_base);

    synchronous_complete_command(index, command_slot_for_identify);

    auto convert_ata_string = [](String& ata)
    {
        for (size_t i = 0; i < ata.size() - 1; i+= 2)
            swap(ata[i], ata[i + 1]);
    };

    auto mapping = TypedMapping<u16>::create("ATA Identify"_sv, identify_base, identify_command_buffer_size);
    u16* data = mapping.get();

    static constexpr size_t serial_number_offset = 10;
    static constexpr size_t serial_number_size = 10;

    static constexpr size_t model_number_offset = 27;
    static constexpr size_t model_number_size = 40;

    static constexpr size_t major_version_offset = 80;
    static constexpr size_t minor_version_offset = 81;

    static constexpr size_t lba_48bit_support_bit = SET_BIT(10);
    static constexpr size_t command_and_feature_set_1_offset = 83;
    static constexpr size_t command_and_features_set_2_offset = 86;

    static constexpr size_t lba_offset = 60;
    static constexpr size_t lba48_offset = 100;

    static constexpr size_t sector_info_offset = 106;

    port.model_string = StringView(reinterpret_cast<char*>(&data[model_number_offset]), model_number_size);
    convert_ata_string(port.model_string);

    port.serial_number = StringView(reinterpret_cast<char*>(&data[serial_number_offset]), serial_number_size);
    convert_ata_string(port.serial_number);

    port.ata_major = data[major_version_offset];
    port.ata_minor = data[minor_version_offset];

    log("AHCI") << "ata version " << port.ata_major << ":" << port.ata_minor;
    log("AHCI") << "model \"" << port.model_string.c_string() << "\"";
    log("AHCI") << "serial number \"" << port.serial_number.c_string() << "\"";

    auto command_and_features = data[command_and_feature_set_1_offset];

    // As per ATA/ATAPI specification: "If bit 14 of word 83 is set to one and bit 15 of word 83
    // is cleared to zero, then the contents of words 82..83 contain valid support information."
    if (IS_BIT_SET(command_and_features, 14) && !IS_BIT_SET(command_and_features, 15)) {
        port.supports_48bit_lba = command_and_features & lba_48bit_support_bit;
    }

    if (!port.supports_48bit_lba) {
        // As per ATA/ATAPI specification: "if bit 14 of word 87 is set to one and bit 15 of word
        // 87 is cleared to zero, then the contents of words 85..87 contain valid information.
        auto word_87 = data[87];

        if (IS_BIT_SET(word_87, 14) && !IS_BIT_SET(word_87, 15)) {
            command_and_features = data[command_and_features_set_2_offset];
            port.supports_48bit_lba = command_and_features & lba_48bit_support_bit;
        }
    }

    if (port.supports_48bit_lba) {
        copy_memory(&data[lba48_offset], &port.lba_count, 8);
    } else {
        copy_memory(&data[lba_offset], &port.lba_count, 4);
    }

    log("AHCI") << "drive lba count " << port.lba_count << " supports 48-bit lba? " << port.supports_48bit_lba;

    // defaults are 256 words per sector
    port.logical_sector_size = 512;
    port.physical_sector_size = 512;

    u16 sector_info = data[sector_info_offset];

    // As per ATA/ATAPI specification: If bit 14 of word 106 is set to one and bit 15 of word 106
    // is cleared to zero, then the contents of word 106 contain valid information.
    if (IS_BIT_SET(sector_info, 14) && !IS_BIT_SET(sector_info, 15)) {
        static constexpr size_t multiple_logical_sectors_per_physical_bit = SET_BIT(13);
        static constexpr size_t logical_sector_greater_than_256_words_bit = SET_BIT(12);
        static constexpr size_t power_of_2_logical_sectors_per_physical_mask = 0b1111;

        size_t logical_sectors_per_physical = 1;

        if (sector_info & multiple_logical_sectors_per_physical_bit)
            logical_sectors_per_physical = 1 << (sector_info & power_of_2_logical_sectors_per_physical_mask);

        if (sector_info & logical_sector_greater_than_256_words_bit) {
            static constexpr size_t logical_sector_size_offset = 117;
            copy_memory(&data[logical_sector_size_offset], &port.logical_sector_size, 4);
        }

        port.physical_sector_size = port.logical_sector_size * logical_sectors_per_physical;

        if (port.logical_sector_size == 512 && port.physical_sector_size != 512) {
            static constexpr size_t alignment_info_offset = 209;
            static constexpr size_t alignment_offset_mask = 0b11111111111111;

            auto alignment_info = data[alignment_info_offset];
            port.lba_offset_into_physical_sector = port.logical_sector_size * (alignment_info & alignment_offset_mask);
        }
    }

    log("AHCI") << "physical sector size: " << port.physical_sector_size;
    log("AHCI") << "logical sector size: " << port.logical_sector_size;
    log("AHCI") << "lba offset into physical: " << port.lba_offset_into_physical_sector;

    MemoryManager::the().free_page(Page(identify_base));
}

void AHCI::synchronous_complete_command(size_t port, size_t slot)
{
    auto command_bit = 1 << slot;

    m_hba->ports[port].command_issue = command_bit;

    static constexpr size_t max_wait_timeout = Time::nanoseconds_in_millisecond * 500;
    auto wait_begin = Timer::nanoseconds_since_boot();
    auto wait_end = wait_begin + max_wait_timeout;

    while (m_hba->ports[port].command_issue & command_bit)
    {
        if (wait_end < Timer::nanoseconds_since_boot())
            break;
    }

    if (m_hba->ports[port].command_issue & command_bit)
        runtime::panic("AHCI: command failed to complete within 500ms");

    panic_if_port_error(port_read<PortInterruptStatus>(port));
}

void AHCI::panic_if_port_error(PortInterruptStatus status)
{
    if (!status.contains_errors())
        return;

    String error_string;
    error_string << "Encountered an unexpected port error: " << status.error_string();
}

void AHCI::enable_ahci()
{
    auto ghc = hba_read<GHC>();

    while (!ghc.ahci_enable) {
        ghc.ahci_enable = true;
        hba_write(ghc);
        ghc = hba_read<GHC>();
    }
}

void AHCI::reset_port(size_t index)
{
    auto sctl = port_read<PortSATAControl>(index);
    sctl.device_detection_initialization = PortSATAControl::DeviceDetectionInitialization::PERFORM_INITIALIZATION;
    port_write(index, sctl);

    static constexpr size_t comreset_delivery_wait = Time::nanoseconds_in_millisecond;
    auto wait_begin = Timer::nanoseconds_since_boot();
    auto wait_end = wait_begin + comreset_delivery_wait;

    while (wait_end > Timer::nanoseconds_since_boot());

    sctl = port_read<PortSATAControl>(index);
    sctl.device_detection_initialization = PortSATAControl::DeviceDetectionInitialization::NOT_REQUESTED;
    port_write(index, sctl);

    wait_begin = Timer::nanoseconds_since_boot();
    wait_end = wait_begin + comreset_delivery_wait;

    auto ssts = port_read<PortSATAStatus>(index);

    while (ssts.device_detection != PortSATAStatus::DeviceDetection::DEVICE_PRESENT_PHY) {
        ssts = port_read<PortSATAStatus>(index);

        if (wait_end < Timer::nanoseconds_since_boot())
            break;
    }

    if (ssts.device_detection != PortSATAStatus::DeviceDetection::DEVICE_PRESENT_PHY)
        runtime::panic("AHCI: Port physical layer failed to come back online after reset");

    // Removing this will cause the driver to break on real hw
    sleep::for_milliseconds(50);

    m_hba->ports[index].error = 0xFFFFFFFF;

    log("AHCI") << "successfully reset port " << index;
}

void AHCI::hba_reset()
{
    bool should_spinup = hba_read<CAP>().supports_staggered_spin_up;

    auto ghc = hba_read<GHC>();
    ghc.hba_reset = true;
    hba_write(ghc);

    static constexpr size_t hba_reset_timeout = 1 * Time::nanoseconds_in_second;
    auto wait_start_ts = Timer::nanoseconds_since_boot();
    auto wait_end_ts = wait_start_ts + hba_reset_timeout;

    do {
        ghc = hba_read<GHC>();
    } while (ghc.hba_reset != 0 && Timer::nanoseconds_since_boot() < wait_end_ts);

    if (ghc.hba_reset)
        runtime::panic("AHCI: HBA failed to reset after 1 second");

    enable_ahci();

    if (should_spinup) {
        auto pi = m_hba->ports_implemented;

        for (size_t i = 0; i < 32; ++i) {
            if (!IS_BIT_SET(pi, i))
                continue;

            auto cas = port_read<PortCommandAndStatus>(i);
            cas.spinup_device = true;
            port_write(i, cas);

            sleep::for_milliseconds(50);
        }
    } else {
        // If staggered spin up is not supported the controller automatically spins every port up
        // So we have to wait for it. I have no idea how to check that it did earlier.
        sleep::for_milliseconds(50);
    }

    log() << "AHCI: succesfully reset HBA";
}

void AHCI::perform_bios_handoff()
{
    if (hba_read<CAP2>().bios_handoff == false)
        return;

    log() << "AHCI: Performing BIOS handoff...";

    auto handoff = hba_read<BIOSHandoffControlAndStatus>();
    handoff.os_owned_semaphore = true;
    hba_write(handoff);

    static constexpr size_t bios_busy_wait_timeout = 25 * Time::nanoseconds_in_millisecond;
    auto wait_start_ts = Timer::nanoseconds_since_boot();
    auto wait_end_ts = wait_start_ts + bios_busy_wait_timeout;

    while (!hba_read<BIOSHandoffControlAndStatus>().bios_busy && Timer::nanoseconds_since_boot() < wait_end_ts)
        ;

    if (!hba_read<BIOSHandoffControlAndStatus>().bios_busy) {
        warning() << "AHCI: BIOS didn't set the busy flag within 25ms so we assume the controller is ours.";
        return;
    }

    static constexpr size_t bios_cleanup_wait_timeout = 2 * Time::nanoseconds_in_second;
    wait_start_ts = Timer::nanoseconds_since_boot();
    wait_end_ts = wait_start_ts + bios_cleanup_wait_timeout;

    while (hba_read<BIOSHandoffControlAndStatus>().bios_busy && Timer::nanoseconds_since_boot() < wait_end_ts)
        ;

    if (hba_read<BIOSHandoffControlAndStatus>().bios_busy)
        warning() << "AHCI: BIOS is still busy after 2 seconds, assuming the controller is ours.";
}

void AHCI::autodetect(const DynamicArray<PCI::DeviceInfo>& devices)
{
    for (const auto& device : devices) {
        if (class_code != device.class_code)
            continue;
        if (subclass_code != device.subclass_code)
            continue;
        if (programming_interface != device.programming_interface)
            continue;

        log() << "AHCI: detected a new controller -> " << device;
        new AHCI(device);
    }
}


Address AHCI::allocate_safe_page()
{
    auto page = MemoryManager::the().allocate_page();

#ifdef ULTRA_64
    if (page.address() > 0xFFFFF000 && !m_supports_64bit)
        runtime::panic("AHCI: Couldn't allocate a suitable page for controller");
#endif

    return page.address();
}

}
