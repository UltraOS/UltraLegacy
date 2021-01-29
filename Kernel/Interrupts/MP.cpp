#include "MP.h"
#include "Common/Logger.h"
#include "Memory/MemoryManager.h"

namespace kernel {

MP::FloatingPointer* MP::s_floating_pointer;

InterruptController::SMPData* MP::parse()
{
    ASSERT(s_floating_pointer == nullptr);

    s_floating_pointer = find_floating_pointer_table();

    if (s_floating_pointer == nullptr)
        return nullptr;

    log() << "MP: Found the floating pointer table at "
          << MemoryManager::virtual_to_physical(s_floating_pointer);

    return parse_configuration_table(s_floating_pointer);
}

MP::FloatingPointer* MP::find_floating_pointer_table()
{
    static constexpr auto mp_floating_pointer_signature = "_MP_"_sv;
    static constexpr size_t table_alignment = 16;

    static constexpr Address ebda_base = 0x80000;
    static constexpr Address ebda_end = 0x9FFFF;
    auto ebda_range = Range::from_two_pointers(MemoryManager::physical_to_virtual(ebda_base), MemoryManager::physical_to_virtual(ebda_end));

    log() << "MP: Trying to find the floating pointer table in the EBDA...";

    auto string = mp_floating_pointer_signature.find_in_range(ebda_range, table_alignment);

    if (!string.empty())
        return Address(string.begin()).as_pointer<FloatingPointer>();

    static constexpr Address bios_rom_base = 0xF0000;
    static constexpr Address bios_rom_end = 0xFFFFF;
    auto bios_rom_range = Range::from_two_pointers(MemoryManager::physical_to_virtual(bios_rom_base), MemoryManager::physical_to_virtual(bios_rom_end));

    log() << "MP: Couldn't find the floating pointer table in the EBDA, trying ROM...";

    string = mp_floating_pointer_signature.find_in_range(bios_rom_range, table_alignment);

    if (!string.empty())
        return Address(string.begin()).as_pointer<FloatingPointer>();

    static constexpr Address bda_address = 0x400;
    static constexpr Address bda_address_linear = MemoryManager::physical_to_virtual(bda_address);
    static constexpr size_t kilobytes_before_ebda_offset = 0x13;

    auto kilobytes_before_ebda = *Address(bda_address_linear + kilobytes_before_ebda_offset).as_pointer<u16>();
    auto last_base_kilobyte = MemoryManager::physical_to_virtual((kilobytes_before_ebda - 1) * KB);
    auto last_base_kilobyte_range = Range(last_base_kilobyte, 1 * KB);

    log() << "MP: Couldn't find the floating pointer table in ROM, trying last 1KB of base memory...";

    string = mp_floating_pointer_signature.find_in_range(last_base_kilobyte_range, table_alignment);

    if (!string.empty())
        return Address(string.begin()).as_pointer<FloatingPointer>();

    warning() << "MP: Couldn't find the floating pointer table, reverting to single core configuration.";

    return nullptr;
}

InterruptController::SMPData* MP::parse_configuration_table(FloatingPointer* fp_table)
{
    static constexpr u8 imcr_register = SET_BIT(7);

    if (fp_table->features & imcr_register) {
        log() << "MP: IMCR register found, switching to APIC mode...";

        static constexpr u8 imcr = 0x70;
        static constexpr u8 select = 0x22;
        static constexpr u8 setting = 0x23;
        static constexpr u8 apic_mode = 1;

        IO::out8<select>(imcr);
        IO::out8<setting>(IO::in8<setting>() | apic_mode);
    }

    // TODO: handle this case
    if (fp_table->default_configuration)
        return nullptr;

    auto configuration_table_linear = MemoryManager::physical_to_virtual(fp_table->configuration_table_pointer);

    auto& configuration_table = *configuration_table_linear.as_pointer<MP::ConfigurationTable>();

    static constexpr auto mp_configuration_table_signature = "PCMP"_sv;

    if (configuration_table.signature != mp_configuration_table_signature) {
        warning() << "MP: incorrect configuration table signature, ignoring the rest of the table...";
        return nullptr;
    }

    log() << "MP: Local APIC at " << format::as_hex << configuration_table.local_apic_pointer;

    Address entry_address = &configuration_table + 1;

    auto* smp_data = new InterruptController::SMPData;
    smp_data->lapic_address = static_cast<ptr_t>(configuration_table.local_apic_pointer);

    u8 isa_bus_id = 0xFF;
    u8 pci_bus_id = 0xFF;

    for (size_t i = 0; i < configuration_table.entry_count; ++i) {
        EntryType type = *entry_address.as_pointer<MP::EntryType>();

        if (type == EntryType::PROCESSOR) {
            auto& processor = *entry_address.as_pointer<ProcessorEntry>();

            bool is_bsp = processor.flags & ProcessorEntry::Flags::BSP;
            bool is_ok = processor.flags & ProcessorEntry::Flags::OK;

            log() << "MP: A new processor -> APIC id:" << processor.local_apic_id
                  << " type:" << (is_bsp ? "BSP" : "AP") << " is_ok:" << is_ok;

            if (is_bsp)
                smp_data->bootstrap_processor_apic_id = processor.local_apic_id;
            else
                smp_data->application_processor_apic_ids.append(processor.local_apic_id);

        } else if (type == EntryType::BUS) {
            auto& bus = *entry_address.as_pointer<BusEntry>();

            log() << "MP: Bus id " << bus.id << " type " << StringView(bus.type_string, 6);

            auto isa_bus = "ISA"_sv;
            auto pci_bus = "PCI"_sv;

            if (StringView(bus.type_string, 3) == isa_bus)
                isa_bus_id = bus.id;
            if (StringView(bus.type_string, 3) == pci_bus)
                pci_bus_id = bus.id;

        } else if (type == EntryType::IO_APIC) {
            auto& io_apic = *entry_address.as_pointer<IOAPICEntry>();

            bool is_ok = io_apic.flags & IOAPICEntry::Flags::OK;

            log() << "MP: I/O APIC at " << format::as_hex << io_apic.io_apic_pointer
                  << " is_ok:" << is_ok << " id:" << io_apic.id;

            smp_data->ioapic_address = static_cast<ptr_t>(io_apic.io_apic_pointer);
        } else if (type == EntryType::IO_INTERRUPT_ASSIGNMENT || type == EntryType::LOCAL_INTERRUPT_ASSIGNMENT) {
            ASSERT(isa_bus_id != 0xFF);

            auto& interrupt = *entry_address.as_pointer<InterruptEntry>();

            const char* str_type = type == EntryType::IO_INTERRUPT_ASSIGNMENT ? "I/O Interrupt Assignment"
                                                                              : "Local Interrupt Assignment";

            const char* str_apic = type == EntryType::IO_INTERRUPT_ASSIGNMENT ? "IOAPIC" : "LAPIC";

            log() << "MP: " << str_type << " entry:"
                  << "\n----> Type: " << InterruptEntry::to_string(interrupt.interrupt_type) << " Interrupt"
                  << "\n----> Polarity: " << InterruptEntry::to_string(interrupt.polarity)
                  << "\n----> Trigger mode: " << InterruptEntry::to_string(interrupt.trigger_mode)
                  << "\n----> Source bus ID: " << interrupt.source_bus_id
                  << "\n----> Source bus IRQ: " << interrupt.source_bus_irq << "\n----> Destination " << str_apic
                  << " id: " << interrupt.destination_ioapic_id << "\n----> Destination " << str_apic
                  << " pin: " << interrupt.destination_ioapic_pin << "\n";

            // TODO: 1. handle lapic assignemnts separetely (LINT0/LINT1 stuff)
            //       2. handle interrupt types other than the default vectored
            //       3. handle interrupts that are coming from the PCI bus
            if (type == EntryType::LOCAL_INTERRUPT_ASSIGNMENT || interrupt.interrupt_type != InterruptEntry::Type::INT
                || interrupt.source_bus_id != isa_bus_id) {
                entry_address += sizeof_entry(type);
                continue;
            }

            // TODO: take care of the possibility of having multiple IOAPICs
            InterruptController::IRQInfo info {};
            info.original_irq_index = interrupt.source_bus_irq;
            info.redirected_irq_index = interrupt.destination_ioapic_pin;

            if (interrupt.polarity != InterruptEntry::Polarity::CONFORMING) {
                info.is_active_high = interrupt.polarity == InterruptEntry::Polarity::ACTIVE_HIGH;
            } else if (interrupt.source_bus_id == isa_bus_id) {
                info.is_active_high = true;
            } else if (interrupt.source_bus_id == pci_bus_id) {
                info.is_active_high = false;
            } else {
                warning()
                    << "MP: interrupt polarity was declared conforming but the default mode for bus "
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
                warning() << "MP: interrupt trigger mode was declared conforming\n"
                             "          but the default mode for bus "
                          << interrupt.source_bus_id << " is unknown.\n";
                entry_address += sizeof_entry(type);
                continue;
            }

            smp_data->irqs.emplace(info);
        }

        entry_address += sizeof_entry(type);
    }

    return smp_data;
}

}
