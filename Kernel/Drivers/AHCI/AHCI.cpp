#include "AHCI.h"
#include "Common/Logger.h"
#include "Drivers/PCI/Access.h"
#include "Interrupts/Timer.h"

#ifdef ULTRA_32

#define SET_DWORDS_TO_ADDRESS(lower, upper, address) lower = address & 0xFFFFFFFF
#define ADDRESS_FROM_TWO_DWORDS(lower, upper) Address(lower)

#elif defined(ULTRA_64)

#define SET_DWORDS_TO_ADDRESS(lower, upper, address) lower = address & 0xFFFFFFFF; upper = address >> 32
#define ADDRESS_FROM_TWO_DWORDS(lower, upper) Address(lower | (static_cast<Address::underlying_pointer_type>(upper) << 32))

#endif

#include "Multitasking/Sleep.h"
namespace kernel {

AHCI::AHCI(PCI::Location location)
    : Device(location)
{
    enable_memory_space();
    make_bus_master();
    clear_interrupts_disabled();

    auto bar_5 = bar(5);
    ASSERT(bar_5.type == BAR::Type::MEMORY);
    m_hba = TypedMapping<volatile HBA>::create("AHCI HBA"_sv, bar_5.address);

    // We have to enable AHCI before touching any AHCI port
    enable_ahci();

    perform_bios_handoff();
    hba_reset();

    auto ghc = hba_read<GHC>();
    ghc.interrupt_enable = true;
    hba_write(ghc);

    auto cap = hba_read<CAP>();
    m_supports_64bit = cap.supports_64bit_addressing;
    m_command_slots_per_port = cap.number_of_command_slots + 1;

    log() << "AHCI: detected " << m_command_slots_per_port << " command slots per port";

    initialize_ports();
}

void AHCI::initialize_ports()
{
    auto implemented_ports = m_hba->ports_implemented;

    log() << "AHCI: detected " << __builtin_popcount(implemented_ports) << " implemented ports";

    for (size_t i = 0; i < 32; ++i) {
        if (!(implemented_ports & SET_BIT(i)))
            continue;

        m_ports[i].implemented = true;
        initialize_port(i);
    }
}

bool AHCI::port_has_device_attached(size_t index) {
    auto status = port_read<PortSATAStatus>(index);

    return !(status.power_management != PortSATAStatus::InterfacePowerManagement::INTERFACE_ACTIVE ||
             status.device_detection != PortSATAStatus::DeviceDetection::DEVICE_PRESENT_PHY);
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

    ensure_port_is_idle(index);

    auto& port = m_ports[index];

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

    // NOTE: we *have* to reset after we set both fis & command list registers,
    //       otherwise the port will not be able to send an init D2H to retrieve
    //       the signature & other info
    reset_port(index);

    auto cmd = port_read<PortCommandAndStatus>(index);
    cmd.fis_receive_enable = true;
    cmd.start = true;
    port_write(index, cmd);

    m_hba->ports[index].error = 0xFFFFFFFF;

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

    log() << "AHCI: successfully intitialized port " << index;
}

void AHCI::ensure_port_is_idle(size_t index)
{
    auto cmd = port_read<PortCommandAndStatus>(index);

    if (!cmd.start && !cmd.command_list_running && !cmd.fis_receive_enable && !cmd.fis_receive_running) {
        log("AHCI") << "port " << index << " is already idle";
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

    log() << "AHCI: succesfully set port " << index << " to idle state";
}

// FIXME: very raw impl
void AHCI::identify_sata_port(size_t index)
{
    auto& port = m_ports[index];
    auto& command_header_0 = port.command_list->commands[0];
    Address identify_base = allocate_safe_page();

    Address command_table_0_phys = ADDRESS_FROM_TWO_DWORDS(command_header_0.command_table_base_address, command_header_0.command_table_base_address_upper);
    auto command_table_0 = TypedMapping<CommandTable>::create("AHCI command table"_sv, command_table_0_phys, sizeof(CommandTable) + sizeof(PRDT));

    command_header_0.physical_region_descriptor_table_length = 1;
    command_header_0.command_fis_length = FISHostToDevice::size_in_dwords;

    FISHostToDevice fis {};
    fis.command_register = 0xEC; // FIXME: unmagic this number
    fis.is_command = true;
    copy_memory(&fis, command_table_0->command_fis, sizeof(fis));

    auto& prdt_0 = command_table_0->prdts[0];
    prdt_0.byte_count = 511; // FIXME: unmagic

    SET_DWORDS_TO_ADDRESS(prdt_0.data_base, prdt_0.data_upper, identify_base);

    m_hba->ports[index].command_issue = 1;

    while (m_hba->ports[index].command_issue & 1) {
        // TODO: handler errors
    }

    auto mapping = TypedMapping<u16>::create("ATA Identify"_sv, identify_base, 512);
    auto* data = mapping.get();

    info() << "Non-zero ATA Identify words:";

    for (size_t i = 0; i < 512; ++i) {
        if (data[i] == 0)
            continue;

        info() << format::as_hex << data[i];
    }

    // TODO: record useful fields

    MemoryManager::the().free_page(Page(identify_base));
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

    auto ssts = port_read<PortSATAStatus>(index);

    do {
        ssts = port_read<PortSATAStatus>(index);
    } while (ssts.device_detection != PortSATAStatus::DeviceDetection::DEVICE_PRESENT_PHY);

    log("AHCI") << "successfully reset port " << index;
}

void AHCI::hba_reset()
{
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
        new AHCI(device.location);
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
