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

    m_supports_64bit = hba_read<CAP>().supports_64bit_addressing;
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

        new AHCI(device.location);
    }
}

}
