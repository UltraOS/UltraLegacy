#include "InterruptController.h"
#include "APIC.h"
#include "Core/CPU.h"
#include "LAPIC.h"
#include "Memory/MemoryManager.h"
#include "PIC.h"

namespace kernel {

InterruptController*          InterruptController::s_instance;
InterruptController::SMPData* InterruptController::s_smp_data;

void InterruptController::discover_and_setup()
{
    // ACPI::find_whatver_tables_are_responsible_for_smp();

    auto* floating_pointer = MP::find_floating_pointer_table();

    if (floating_pointer) {
        log() << "InterruptController: Found the floating pointer table at "
              << MemoryManager::virtual_to_physical(floating_pointer);

        MP::parse_configuration_table(floating_pointer);

        log() << "InteruptController: Initializing LAPIC and IOAPIC...";

        s_instance = new APIC;

        return;
    }

    log() << "InterruptController: No APIC support detected, reverting to PIC";
    s_instance = new PIC;
}

InterruptController& InterruptController::the()
{
    ASSERT(s_instance != nullptr);

    return *s_instance;
}

Address InterruptController::find_string_in_range(Address begin, Address end, size_t step, StringView string)
{
    for (ptr_t pointer = begin; pointer < end; pointer += step) {
        if (StringView(Address(pointer).as_pointer<const char>(), string.size()) == string)
            return pointer;
    }

    return nullptr;
}

InterruptController::MP::FloatingPointer* InterruptController::MP::find_floating_pointer_table()
{
    static constexpr auto   mp_floating_pointer_signature = "_MP_"_sv;
    static constexpr size_t table_alignment               = 16;

    static constexpr Address ebda_base        = 0x80000;
    static constexpr Address ebda_end         = 0x9FFFF;
    static constexpr Address ebda_base_linear = MemoryManager::physical_to_virtual(ebda_base);
    static constexpr Address ebda_end_linear  = MemoryManager::physical_to_virtual(ebda_end);

    log() << "InterruptController: Trying to find the floating pointer table in the EBDA...";

    auto address
        = find_string_in_range(ebda_base_linear, ebda_end_linear, table_alignment, mp_floating_pointer_signature);

    if (address)
        return MemoryManager::physical_to_virtual(address).as_pointer<MP::FloatingPointer>();

    static constexpr Address bios_rom_base        = 0xF0000;
    static constexpr Address bios_rom_end         = 0xFFFFF;
    static constexpr Address bios_rom_base_linear = MemoryManager::physical_to_virtual(bios_rom_base);
    static constexpr Address bios_rom_end_linear  = MemoryManager::physical_to_virtual(bios_rom_end);

    log() << "InterruptController: Couldn't find the floating pointer table in the EBDA, trying ROM...";

    address = find_string_in_range(bios_rom_base_linear,
                                   bios_rom_end_linear,
                                   table_alignment,
                                   mp_floating_pointer_signature);

    if (address)
        return address.as_pointer<FloatingPointer>();

    static constexpr Address bda_address                  = 0x400;
    static constexpr Address bda_address_linear           = MemoryManager::physical_to_virtual(bda_address);
    static constexpr size_t  kilobytes_before_ebda_offset = 0x13;

    auto kilobytes_before_ebda = *Address(bda_address_linear + kilobytes_before_ebda_offset).as_pointer<u16>();
    auto last_base_kilobyte    = MemoryManager::physical_to_virtual((kilobytes_before_ebda - 1) * KB);

    log() << "InterruptController: Couldn't find the floating pointer table in ROM, trying last 1KB of base memory...";

    address = find_string_in_range(last_base_kilobyte,
                                   last_base_kilobyte + 1 * KB,
                                   table_alignment,
                                   mp_floating_pointer_signature);

    if (address)
        return MemoryManager::physical_to_virtual(address).as_pointer<MP::FloatingPointer>();

    warning()
        << "InterruptController: Couldn't find the floating pointer table, reverting to single core configuration.";

    return nullptr;
}

void InterruptController::MP::parse_configuration_table(FloatingPointer* fp_table)
{
    static constexpr u8 imcr_register = SET_BIT(7);

    if (fp_table->features & imcr_register) {
        log() << "InterruptController: IMCR register found, switching to APIC mode...";

        static constexpr u8 imcr      = 0x70;
        static constexpr u8 select    = 0x22;
        static constexpr u8 setting   = 0x23;
        static constexpr u8 apic_mode = 1;

        IO::out8<select>(imcr);
        IO::out8<setting>(IO::in8<setting>() | apic_mode);
    }

    if (fp_table->default_configuration) {
        // TODO: handle the default configuration as well, its 2 processors and all default mappings
        log() << "InterruptController: default mp configuration was set, ignoring the rest of the table.";
        return;
    }

    auto configuration_table_linear = MemoryManager::physical_to_virtual(fp_table->configuration_table_pointer.raw());

    auto& configuration_table = *configuration_table_linear.as_pointer<MP::ConfigurationTable>();

    static constexpr auto mp_configuration_table_signature = "PCMP"_sv;

    if (configuration_table.signature != mp_configuration_table_signature) {
        warning() << "InterruptController: incorrect configuration table signature, ignoring the rest of the table...";
        return;
    }

    log() << "InterruptController: Local APIC at " << format::as_hex << configuration_table.local_apic_pointer.raw();

    Address entry_address = &configuration_table + 1;

    s_smp_data                = new SMPData;
    s_smp_data->lapic_address = static_cast<ptr_t>(configuration_table.local_apic_pointer);

    u8 isa_bus_id = 0xFF;
    u8 pci_bus_id = 0xFF;

    for (size_t i = 0; i < configuration_table.entry_count; ++i) {
        EntryType type = *entry_address.as_pointer<MP::EntryType>();

        if (type == EntryType::PROCESSOR) {
            auto& processor = *entry_address.as_pointer<ProcessorEntry>();

            bool is_bsp = processor.flags & ProcessorEntry::Flags::BSP;
            bool is_ok  = processor.flags & ProcessorEntry::Flags::OK;

            log() << "InterruptController: A new processor -> APIC id:" << processor.local_apic_id
                  << " type:" << (is_bsp ? "BSP" : "AP") << " is_ok:" << is_ok;

            if (is_bsp)
                s_smp_data->bootstrap_processor_apic_id = processor.local_apic_id;
            else
                s_smp_data->application_processor_apic_ids.append(processor.local_apic_id);

        } else if (type == EntryType::BUS) {
            auto& bus = *entry_address.as_pointer<BusEntry>();

            log() << "InterruptController: Bus id " << bus.id << " type " << StringView(bus.type_string, 6);

            auto isa_bus = "ISA"_sv;
            auto pci_bus = "PCI"_sv;

            if (StringView(bus.type_string, 3) == isa_bus)
                isa_bus_id = bus.id;
            if (StringView(bus.type_string, 3) == pci_bus)
                pci_bus_id = bus.id;

        } else if (type == EntryType::IO_APIC) {
            auto& io_apic = *entry_address.as_pointer<IOAPICEntry>();

            bool is_ok = io_apic.flags & IOAPICEntry::Flags::OK;

            log() << "InterruptController: I/O APIC at " << format::as_hex << io_apic.io_apic_pointer.raw()
                  << " is_ok:" << is_ok << " id:" << io_apic.id;

            s_smp_data->ioapic_address = static_cast<ptr_t>(io_apic.io_apic_pointer);
        } else if (type == EntryType::IO_INTERRUPT_ASSIGNMENT || type == EntryType::LOCAL_INTERRUPT_ASSIGNMENT) {
            ASSERT(isa_bus_id != 0xFF);

            auto& interrupt = *entry_address.as_pointer<InterruptEntry>();

            // TODO: 1. handle lapic assignemnts separetely (LINT0/LINT1 stuff)
            //       2. handle interrupt types other than the default vectored
            //       3. handle interrupts that are coming from the PCI bus
            if (type == EntryType::LOCAL_INTERRUPT_ASSIGNMENT || interrupt.interrupt_type != InterruptEntry::Type::INT
                || interrupt.source_bus_id != isa_bus_id) {
                entry_address += sizeof_entry(type);
                continue;
            }

            const char* str_type = type == EntryType::IO_INTERRUPT_ASSIGNMENT ? "I/O Interrupt Assignment"
                                                                              : "Local Interrupt Assignment";

            const char* str_apic = type == EntryType::IO_INTERRUPT_ASSIGNMENT ? "IOAPIC" : "LAPIC";

            log() << "InterruptController: " << str_type << " entry:"
                  << "\n----> Type: " << InterruptEntry::to_string(interrupt.interrupt_type) << " Interrupt"
                  << "\n----> Polarity: " << InterruptEntry::to_string(interrupt.polarity)
                  << "\n----> Trigger mode: " << InterruptEntry::to_string(interrupt.trigger_mode)
                  << "\n----> Source bus ID: " << interrupt.source_bus_id
                  << "\n----> Source bus IRQ: " << interrupt.source_bus_irq << "\n----> Destination " << str_apic
                  << " id: " << interrupt.destination_ioapic_id << "\n----> Destination " << str_apic
                  << " pin: " << interrupt.destination_ioapic_pin << "\n";

            // TODO: take care of the possibility of having multiple IOAPICs
            IRQInfo info {};
            info.original_irq_index   = interrupt.source_bus_irq;
            info.redirected_irq_index = interrupt.destination_ioapic_pin;

            if (interrupt.polarity != InterruptEntry::Polarity::CONFORMING) {
                info.is_active_high = interrupt.polarity == InterruptEntry::Polarity::ACTIVE_HIGH;
            } else if (interrupt.source_bus_id == isa_bus_id) {
                info.is_active_high = true;
            } else if (interrupt.source_bus_id == pci_bus_id) {
                info.is_active_high = false;
            } else {
                warning()
                    << "InterruptController: interrupt polarity was declared conforming but the default mode for bus "
                    << interrupt.source_bus_id << " is unknown.\n";
                entry_address += sizeof_entry(type);
                continue;
            }

            if (interrupt.trigger_mode != InterruptEntry::TriggerMode::CONFORMING) {
                info.is_edge = interrupt.trigger_mode == InterruptEntry::TriggerMode::EDGE;
            } else if (interrupt.source_bus_id == isa_bus_id) {
                info.is_edge = true;
            } else if (interrupt.source_bus_id == pci_bus_id) {
                info.is_edge = false;
            } else {
                warning() << "InterruptController: interrupt trigger mode was declared conforming\n"
                             "          but the default mode for bus "
                          << interrupt.source_bus_id << " is unknown.\n";
                entry_address += sizeof_entry(type);
                continue;
            }

            s_smp_data->irqs.emplace(info);
        }

        entry_address += sizeof_entry(type);
    }
}
}
