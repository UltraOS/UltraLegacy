#include "CPU.h"
#include "Interrupts/Common.h"
#include "Interrupts/LAPIC.h"
#include "Memory/MemoryManager.h"

namespace kernel {

CPU::MP::FloatingPointer* CPU::MP::s_floating_pointer;
CPU::SMPData*             CPU::s_smp_data;

void CPU::initialize()
{
    MP::find_floating_pointer_table();

    if (MP::s_floating_pointer) {
        log() << "CPU: Found the floating pointer table at " << MP::s_floating_pointer;

        MP::parse_configuration_table();
    }
}

CPU::EFLAGS CPU::flags()
{
    EFLAGS flags;
    asm volatile("pushf\n"
                 "pop %0\n"
                 : "=a"(flags)::"memory");

    return flags;
}

void CPU::start_all_processors()
{
    ASSERT(s_smp_data != nullptr);

    for (auto processor_id: s_smp_data->application_processor_apic_ids)
        LAPIC::start_processor(processor_id);
}

Address CPU::find_string_in_range(Address begin, Address end, size_t step, StringView string)
{
    for (ptr_t pointer = begin; pointer < end; pointer += step) {
        if (StringView(Address(pointer).as_pointer<const char>(), string.size()) == string)
            return pointer;
    }

    return nullptr;
}

void CPU::MP::find_floating_pointer_table()
{
    static constexpr auto   mp_floating_pointer_signature = "_MP_"_sv;
    static constexpr size_t table_alignment               = 16;

    static constexpr Address ebda_base        = 0x80000;
    static constexpr Address ebda_end         = 0x9FFFF;
    static constexpr Address ebda_base_linear = MemoryManager::physical_address_as_kernel(ebda_base);
    static constexpr Address ebda_end_linear  = MemoryManager::physical_address_as_kernel(ebda_end);

    log() << "CPU: Trying to find the floating pointer table in the EBDA...";

    auto address
        = find_string_in_range(ebda_base_linear, ebda_end_linear, table_alignment, mp_floating_pointer_signature);

    if (address) {
        s_floating_pointer = address.as_pointer<MP::FloatingPointer>();
        return;
    }

    static constexpr Address bios_rom_base        = 0xF0000;
    static constexpr Address bios_rom_end         = 0xFFFFF;
    static constexpr Address bios_rom_base_linear = MemoryManager::physical_address_as_kernel(bios_rom_base);
    static constexpr Address bios_rom_end_linear  = MemoryManager::physical_address_as_kernel(bios_rom_end);

    log() << "CPU: Couldn't find the floating pointer table in the EBDA, trying ROM...";

    address = find_string_in_range(bios_rom_base_linear,
                                   bios_rom_end_linear,
                                   table_alignment,
                                   mp_floating_pointer_signature);

    if (address) {
        s_floating_pointer = address.as_pointer<MP::FloatingPointer>();
        return;
    }

    static constexpr Address bda_address                  = 0x400;
    static constexpr Address bda_address_linear           = MemoryManager::physical_address_as_kernel(bda_address);
    static constexpr size_t  kilobytes_before_ebda_offset = 0x13;

    auto kilobytes_before_ebda = *Address(bda_address_linear + kilobytes_before_ebda_offset).as_pointer<u16>();
    auto last_base_kilobyte    = MemoryManager::physical_address_as_kernel((kilobytes_before_ebda - 1) * KB);

    log() << "CPU: Couldn't find the floating pointer table in ROM, trying last 1KB of base memory...";

    address = find_string_in_range(last_base_kilobyte,
                                   last_base_kilobyte + 1 * KB,
                                   table_alignment,
                                   mp_floating_pointer_signature);

    if (address) {
        s_floating_pointer = address.as_pointer<MP::FloatingPointer>();
        return;
    }

    warning() << "CPU: Couldn't find the floating pointer table, reverting to single core configuration.";
}

void CPU::MP::parse_configuration_table()
{
    ASSERT(s_floating_pointer != nullptr);

    if (s_floating_pointer->default_configuration) {
        log() << "CPU: default mp configuration was set, ignoring the rest of the table.";
        return;
    }

    auto configuration_table_linear
        = MemoryManager::physical_address_as_kernel(s_floating_pointer->configuration_table_pointer);

    auto& configuration_table = *configuration_table_linear.as_pointer<MP::ConfigurationTable>();

    static constexpr auto mp_configuration_table_signature = "PCMP"_sv;

    if (configuration_table.signature != mp_configuration_table_signature) {
        warning() << "CPU: incorrect configuration table signature, ignoring the rest of the table...";
        return;
    }

    log() << "CPU: Local APIC at " << configuration_table.local_apic_pointer;

    Address entry_address = &configuration_table + 1;

    s_smp_data                     = new SMPData;
    s_smp_data->local_apic_address = configuration_table.local_apic_pointer;

    for (size_t i = 0; i < configuration_table.entry_count; ++i) {
        EntryType type = *entry_address.as_pointer<MP::EntryType>();

        if (type == EntryType::PROCESSOR) {
            auto& processor = *entry_address.as_pointer<ProcessorEntry>();

            bool is_bsp = processor.flags & ProcessorEntry::Flags::BSP;
            bool is_ok  = processor.flags & ProcessorEntry::Flags::OK;

            log() << "CPU: A new processor -> APIC id:" << processor.local_apic_id
                  << " type:" << (is_bsp ? "BSP" : "AP") << " is_ok:" << is_ok;

            if (is_bsp)
                s_smp_data->bootstrap_processor_apic_id = processor.local_apic_id;
            else
                s_smp_data->application_processor_apic_ids.append(processor.local_apic_id);

        } else if (type == EntryType::BUS) {
            auto& bus = *entry_address.as_pointer<BusEntry>();

            log() << "CPU: Bus id " << bus.id << " type " << StringView(bus.type_string, 6);

        } else if (type == EntryType::IO_APIC) {
            auto& io_apic = *entry_address.as_pointer<IOAPICEntry>();

            bool is_ok = io_apic.flags & IOAPICEntry::Flags::OK;

            log() << "CPU: I/O APIC at " << io_apic.io_apic_pointer << " is_ok:" << is_ok << " id:" << io_apic.id;

            s_smp_data->io_apic_address = io_apic.io_apic_pointer;
        } else if (type == EntryType::IO_INTERRUPT_ASSIGNMENT || type == EntryType::LOCAL_INTERRUPT_ASSIGNMENT) {
            auto& interrupt = *entry_address.as_pointer<InterruptEntry>();

            const char* str_type = type == EntryType::IO_INTERRUPT_ASSIGNMENT ? "I/O Interrupt Assignment"
                                                                              : "Local Interrupt Assignment";

            const char* str_apic = type == EntryType::IO_INTERRUPT_ASSIGNMENT ? "IOAPIC" : "LAPIC";

            log() << "CPU: " << str_type << " entry:"
                  << "\n----> Type: " << InterruptEntry::to_string(interrupt.interrupt_type) << " Interrupt"
                  << "\n----> Polarity: " << InterruptEntry::to_string(interrupt.polarity)
                  << "\n----> Trigger mode: " << InterruptEntry::to_string(interrupt.trigger_mode)
                  << "\n----> Source bus ID: " << interrupt.source_bus_id
                  << "\n----> Source bus IRQ: " << interrupt.source_bus_irq << "\n----> Destination " << str_apic
                  << " id: " << interrupt.destination_ioapic_id << "\n----> Destination " << str_apic
                  << " pin: " << interrupt.destination_ioapic_pin;
        }

        entry_address += sizeof_entry(type);
    }
}
}
