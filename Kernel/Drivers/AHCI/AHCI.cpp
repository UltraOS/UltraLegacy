#include "AHCI.h"
#include "Common/Logger.h"
#include "Drivers/PCI/Access.h"
#include "Interrupts/Timer.h"

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

    log() << "AHCI: detected " << m_command_slots_per_port + 1 << " command slots per port";

    initialize_ports();
}

void AHCI::initialize_ports()
{
    auto implemented_ports = m_hba->ports_implemented;

    log() << "AHCI: detected " << __builtin_popcount(implemented_ports) << " implemented ports";

    for (size_t i = 0; i < 32; ++i) {
        if (!(implemented_ports & SET_BIT(i)))
            continue;

        initialize_port(i);
    }
}

void AHCI::initialize_port(size_t index)
{
    ensure_port_is_idle(index);

    auto& port = m_ports.emplace();

    auto command_list_page = MemoryManager::the().allocate_page();
    auto base_fis_page = MemoryManager::the().allocate_page();

#ifdef ULTRA_64
    if ((command_list_page.address() > 0xFFFFF000 || base_fis_page.address() > 0xFFFFF000) && !m_supports_64bit)
        runtime::panic("AHCI: Couldn't allocate a suitable page for controller");
#endif

    auto& this_port = m_hba->ports[index];

    this_port.command_list_base_address = command_list_page.address() & 0xFFFFFFFF;
    this_port.fis_base_address = base_fis_page.address() & 0xFFFFFFFF;

#ifdef ULTRA_64
    this_port.command_list_base_address_upper = command_list_page.address() >> 32;
    this_port.fis_base_address_upper = base_fis_page.address() >> 32;
#else
    this_port.command_list_base_address_upper = 0;
    this_port.fis_base_address_upper = 0;
#endif

    port.command_list = TypedMapping<CommandList>::create("AHCI Command List"_sv, command_list_page.address());
    port.hba_fis = TypedMapping<HBAFIS>::create("AHCI Port FIS"_sv, base_fis_page.address());

    auto cmd = port_read<PortCommandAndStatus>(index);
    cmd.fis_receive_enable = true;
    cmd.start = true;
    port_write(index, cmd);

    m_hba->ports[index].error = 0xFFFFFFFF;

    // TODO: fill the command list with entries
    // TODO: ATA_IDENTIFY

    log() << "AHCI: successfully intitialized port " << index;
}

void AHCI::ensure_port_is_idle(size_t index)
{
    auto cmd = port_read<PortCommandAndStatus>(index);

    static constexpr size_t port_wait_timeout = 500 * Time::nanoseconds_in_millisecond;
    Time::time_t wait_start_ts;
    Time::time_t wait_end_ts;

    if (cmd.start) {
        cmd.start = false;
        port_write(index, cmd);

        wait_start_ts = Timer::nanoseconds_since_boot();
        wait_end_ts = wait_start_ts + port_wait_timeout;

        do {
            cmd = port_read<PortCommandAndStatus>(index);
        } while (cmd.command_list_running && Timer::nanoseconds_since_boot() < wait_end_ts);

        if (cmd.command_list_running)
            runtime::panic("AHCI: a port failed to enter idle state in 500ms");
    }

    if (cmd.fis_receive_enable) {
        cmd.fis_receive_enable = false;
        port_write(index, cmd);

        wait_start_ts = Timer::nanoseconds_since_boot();
        wait_end_ts = wait_start_ts + port_wait_timeout;

        do {
            cmd = port_read<PortCommandAndStatus>(index);
        } while (cmd.fis_receive_running && Timer::nanoseconds_since_boot() < wait_end_ts);

        if (cmd.fis_receive_running)
            runtime::panic("AHCI: a port failed to enter idle state in 500ms");
    }

    log() << "AHCI: succesfully set port " << index << " to idle state";
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

}
