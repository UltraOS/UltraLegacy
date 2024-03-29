#include "AHCI.h"
#include "AHCIPort.h"
#include "Common/Logger.h"
#include "Drivers/MMIOHelpers.h"
#include "Drivers/PCI/Access.h"
#include "Interrupts/Timer.h"
#include "Memory/Utilities.h"
#include "Multitasking/Scheduler.h"
#include "Structures.h"

#define AHCI_LOG log("AHCI")

#ifdef AHCI_DEBUG_MODE
#define AHCI_DEBUG log("AHCI")
#else
#define AHCI_DEBUG DummyLogger()
#endif

namespace kernel {

AHCI::AHCI(const PCI::DeviceInfo& info)
    : ::kernel::Device(Category::CONTROLLER)
    , PCI::Device(info)
    , IRQHandler(IRQHandler::Type::MSI)
    , m_ports()
{
    auto bar_5 = bar(5);
    ASSERT(bar_5.type == BAR::Type::MEMORY32);
    m_hba = TypedMapping<volatile HBA>::create("AHCI HBA"_sv, bar_5.address);

    set_command_register(MemorySpaceMode::ENABLED, IOSpaceMode::UNTOUCHED, BusMasterMode::ENABLED, InterruptDisableMode::CLEARED);

    // We have to enable AHCI before touching any AHCI registers
    enable_ahci();

    perform_bios_handoff();
    disable_dma_engines_for_all_ports();
    hba_reset();

    auto ghc = hba_read<GHC>();
    ghc.interrupt_enable = true;
    hba_write(ghc);

    AHCI_LOG << "enabling MSI for interrupt vector " << interrupt_vector();
    enable_msi(interrupt_vector());

    auto cap = hba_read<CAP>();
    m_supports_64bit = cap.supports_64bit_addressing;
    m_command_slots_per_port = cap.number_of_command_slots + 1;

    AHCI_LOG << "detected " << m_command_slots_per_port << " command slots per port";

    initialize_ports();
}

template <typename T>
size_t AHCI::offset_of_port_structure(size_t index)
{
    Address base = m_hba.get();
    base += HBA::offset_to_ports;
    base += HBA::Port::size * index;
    base += T::offset;

    return base;
}

template <typename T>
T AHCI::hba_read()
{
    Address base = m_hba.get();
    base += T::offset;

    return mmio_read32<T>(base);
}

template <typename T>
T AHCI::port_read(size_t index)
{
    Address base = offset_of_port_structure<T>(index);
    return mmio_read32<T>(base);
}

template <typename T>
void AHCI::hba_write(T reg)
{
    Address base = m_hba.get();
    base += T::offset;

    mmio_write32(base, reg);
}

template <typename T>
void AHCI::port_write(size_t index, T reg)
{
    Address base = offset_of_port_structure<T>(index);
    mmio_write32(base, reg);
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

    AHCI_LOG << "detected " << __builtin_popcount(implemented_ports) << " implemented ports";

    for (size_t i = 0; i < 32; ++i) {
        if (!(implemented_ports & SET_BIT(i)))
            continue;

        m_ports[i].implemented = true;
        initialize_port(i);
    }
}

bool AHCI::port_has_device_attached(size_t index)
{
    auto status = port_read<PortSATAStatus>(index);

    return status.power_management == PortSATAStatus::InterfacePowerManagement::INTERFACE_ACTIVE && status.device_detection == PortSATAStatus::DeviceDetection::DEVICE_PRESENT_PHY;
}

AHCI::PortState::Type AHCI::type_of_port(size_t index)
{
    auto& port = m_hba->ports[index];

    switch (static_cast<PortState::Type>(port.signature)) {
    case PortState::Type::SATA:
    case PortState::Type::ATAPI:
    case PortState::Type::ENCLOSURE_MANAGEMENT_BRIDGE:
    case PortState::Type::PORT_MULTIPLIER:
        return static_cast<PortState::Type>(port.signature);
    default:
        return PortState::Type::NONE;
    }
}

void AHCI::initialize_port(size_t index)
{
    if (!port_has_device_attached(index))
        return;

    disable_dma_engines_for_port(index);

    auto& port = m_ports[index];
    port.has_device_attached = true;

    // FIXME: phys memory leak here
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

    reset_port(index);

    enable_dma_engines_for_port(index);

    port.type = type_of_port(index);

    if (port.type == PortState::Type::NONE) {
        AHCI_DEBUG << "port " << index << " doesn't have a device attached";
        return;
    } else if (port.type != PortState::Type::SATA) {
        AHCI_LOG << "port " << index << " has type \"" << port.type_to_string() << "\", which is currently not supported";

        if (port.type == PortState::Type::ATAPI)
            port.medium_type = StorageDevice::Info::MediumType::CD;

        return;
    } else {
        AHCI_LOG << "detected device of type \"" << port.type_to_string() << "\" on port " << index;
    }

    identify_sata_port(index);

    // Clear all previously issued interrupts
    m_hba->ports[index].interrupt_status = 0xFFFFFFFF;

    // Enable all interrupts
    m_hba->ports[index].interrupt_enable = 0xFFFFFFFF;

    m_ports[index].slot_map.set_size(m_command_slots_per_port);

    AHCI_LOG << "successfully intitialized port " << index;
    (new AHCIPort(*this, index))->make_child_of(this);
}

void AHCI::disable_dma_engines_for_port(size_t index)
{
    auto cmd = port_read<PortCommandAndStatus>(index);

    if (!cmd.start && !cmd.command_list_running && !cmd.fis_receive_enable && !cmd.fis_receive_running) {
        AHCI_DEBUG << "DMA engines for port " << index << " are already off";
        return;
    }

    cmd.start = false;
    port_write(index, cmd);

    static constexpr size_t port_wait_timeout = 500 * Time::nanoseconds_in_millisecond;
    auto wait_start_ts = Timer::nanoseconds_since_boot();
    auto wait_end_ts = wait_start_ts + port_wait_timeout;

    do {
        cmd = port_read<PortCommandAndStatus>(index);
    } while (cmd.command_list_running && Timer::nanoseconds_since_boot() < wait_end_ts);

    if (cmd.command_list_running)
        runtime::panic("AHCI: a port failed to enter idle state in 500ms");

    // "Software shall not clear this bit while PxCMD.ST or PxCMD.CR is set to 1."
    cmd = port_read<PortCommandAndStatus>(index);
    cmd.fis_receive_enable = false;
    port_write(index, cmd);

    wait_start_ts = Timer::nanoseconds_since_boot();
    wait_end_ts = wait_start_ts + port_wait_timeout;

    do {
        cmd = port_read<PortCommandAndStatus>(index);
    } while (cmd.fis_receive_running && Timer::nanoseconds_since_boot() < wait_end_ts);

    if (cmd.fis_receive_running)
        runtime::panic("AHCI: a port failed to enter idle state in 500ms");

    AHCI_DEBUG << "successfully disabled DMA engines for port " << index;
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

bool AHCI::handle_irq(RegisterState&)
{
    deferred_invoke();

    return true;
}

bool AHCI::handle_deferred_irq()
{
    volatile auto port_status = m_hba->interrupt_pending_status;

    if (port_status == 0)
        return false;

    for (size_t i = 0; i < 32; ++i) {
        if (!IS_BIT_SET(port_status, i))
            continue;

        auto interrupt_status = port_read<PortInterruptStatus>(i);
        port_write(i, interrupt_status);

        panic_if_port_error(interrupt_status, port_read<PortSATAError>(i));

        handle_port_irq(i);
    }

    m_hba->interrupt_pending_status = port_status;

    return true;
}

void AHCI::handle_port_irq(size_t port_index)
{
    AHCI_DEBUG << "handling IRQ for port " << port_index;

    auto& port = m_ports[port_index];
    LOCK_GUARD(port.state_access_lock);

    volatile auto command_status = m_hba->ports[port_index].command_issue;

    for (size_t bit = 0; bit < m_command_slots_per_port; ++bit) {
        // A command has been completed
        if (port.slot_map.bit_at(bit) && !IS_BIT_SET(command_status, bit)) {
            AHCI_DEBUG << "command " << bit << " is completed";

            auto* req = port.slot_to_request[bit];

            if (!req)
                continue;

            // Complete current OP
            req->queued_ops.pop_front();

            // We're done here
            if (req->queued_ops.empty()) {
                port.slot_to_request[bit] = nullptr;
                req->request->complete(ErrorCode::NO_ERROR);
                delete req;

                // Lets check if we have any other queued requests
                if (port.request_queue.empty()) {
                    // No pending requests, so this slot is now free :)
                    port.deallocate_slot(bit);
                    continue;
                }

                // We had a pending request, set it to the current slot and execute
                auto& new_req = port.request_queue.pop_front();
                port.slot_to_request[bit] = &new_req;

                auto& op = new_req.queued_ops.front();
                op.command_slot = bit;
                execute(op);
                continue;
            }

            // Request has even more OPs
            auto& op = req->queued_ops.front();
            op.command_slot = bit;
            execute(op);
            continue;
        }
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
    auto command_table_0 = TypedMapping<CommandTable>::create("AHCI command table"_sv, command_table_0_phys, sizeof(CommandTable) + sizeof(PRDTEntry));

    command_header_0.physical_region_descriptor_table_length = 1;
    command_header_0.command_fis_length = FISHostToDevice::size_in_dwords;

    FISHostToDevice fis {};
    fis.command_register = ATACommand::IDENTIFY_DEVICE;
    fis.is_command = true;
    copy_memory(&fis, command_table_0->command_fis, sizeof(fis));

    auto& prdt_0 = command_table_0->prdt_entries[0];
    prdt_0.byte_count = identify_command_buffer_size - 1;

    SET_DWORDS_TO_ADDRESS(prdt_0.data_base, prdt_0.data_upper, identify_base);

    synchronous_wait_for_command_completion(index, command_slot_for_identify);

    auto convert_ata_string = [](String& ata) {
        for (size_t i = 0; i < ata.size() - 1; i += 2)
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

    static constexpr size_t media_rotation_rate_offset = 217;

    port.model_string = StringView(reinterpret_cast<char*>(&data[model_number_offset]), model_number_size);
    convert_ata_string(port.model_string);
    port.model_string.strip();

    port.serial_number = StringView(reinterpret_cast<char*>(&data[serial_number_offset]), serial_number_size);
    convert_ata_string(port.serial_number);
    port.serial_number.strip();

    port.ata_major = data[major_version_offset];
    port.ata_minor = data[minor_version_offset];

    AHCI_LOG << "ata version " << port.ata_major << ":" << port.ata_minor;
    AHCI_LOG << "model \"" << port.model_string.c_string() << "\"";
    AHCI_LOG << "serial number \"" << port.serial_number.c_string() << "\"";

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
        copy_memory(&data[lba48_offset], &port.logical_block_count, 8);
    } else {
        copy_memory(&data[lba_offset], &port.logical_block_count, 4);
    }

    AHCI_LOG << "drive lba count " << port.logical_block_count << " supports 48-bit lba? " << port.supports_48bit_lba;

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
            logical_sectors_per_physical = SET_BIT(sector_info & power_of_2_logical_sectors_per_physical_mask);

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

    static constexpr size_t non_rotating_medium_type = 0x0001;
    if (data[media_rotation_rate_offset] == non_rotating_medium_type)
        port.medium_type = StorageDevice::Info::MediumType::SSD;
    else
        port.medium_type = StorageDevice::Info::MediumType::HDD;

    AHCI_LOG << "device rotation rate: " << data[media_rotation_rate_offset];
    AHCI_LOG << "physical sector size: " << port.physical_sector_size;
    AHCI_LOG << "logical sector size: " << port.logical_sector_size;
    AHCI_LOG << "lba offset into physical: " << port.lba_offset_into_physical_sector;

    MemoryManager::the().free_page(Page(identify_base));
}

void AHCI::read_synchronous(size_t port, Address into_virtual_address, LBARange range)
{
    auto ops = build_ops(port, OP::Type::READ, into_virtual_address, range, false);
    synchronous_execute(port, ops);
}

void AHCI::write_synchronous(size_t port, Address from_virtual_address, LBARange range)
{
    auto ops = build_ops(port, OP::Type::WRITE, from_virtual_address, range, false);
    synchronous_execute(port, ops);
}

void AHCI::synchronous_execute(size_t port_index, List<OP>& ops)
{
    auto& port = m_ports[port_index];

    // Spin forever until we can allocate a slot
    Optional<size_t> slot {};
    while (!slot) {
        {
            LOCK_GUARD(port.state_access_lock);
            slot = port.allocate_slot();
        }

        if (!slot.has_value())
            Scheduler::the().yield();
    }

    while (!ops.empty()) {
        auto& op = ops.front();
        op.command_slot = slot.value();

        execute(op);

        ops.pop_front();
    }

    LOCK_GUARD(port.state_access_lock);
    port.deallocate_slot(slot.value());
}

void AHCI::process_async_request(size_t port_index, StorageDevice::AsyncRequest& request)
{
    using RT = StorageDevice::AsyncRequest::OP;
    using OT = OP::Type;

    auto& port = m_ports[port_index];

    auto ops = build_ops(port_index, request.op() == RT::READ ? OT::READ : OT::WRITE, request.virtual_address(), request.lba_range(), true);

    auto& qr = *new PortState::QueuedRequest();
    qr.request = &request;
    qr.queued_ops = move(ops);

    Optional<size_t> slot;
    {
        LOCK_GUARD(port.state_access_lock);
        slot = port.allocate_slot();

        if (!slot) {
            // All slots were busy, our request will be automatically launched as soon as a slot is released.
            port.request_queue.insert_back(qr);
            return;
        }
    }

    qr.queued_ops.front().command_slot = slot.value();
    port.slot_to_request[slot.value()] = &qr;
    execute(qr.queued_ops.front());
}

List<AHCI::OP> AHCI::build_ops(size_t port, OP::Type op, Address virtual_address, LBARange lba_range, bool is_async)
{
    auto& state = state_of_port(port);
    ASSERT(state.logical_block_count >= lba_range.length());

    // PRDT entry limitation (22 bits for byte count)
    size_t max_bytes_per_op = 4 * MB;

    // 28 byte DMA_READ ATA command only supports 256 sector reads/writes per command :(
    if (!state.supports_48bit_lba)
        max_bytes_per_op = state.logical_sector_size * 256;

    size_t bytes_to_read = lba_range.length() * state.logical_sector_size;
    auto ranges = accumulate_contiguous_physical_ranges(virtual_address, bytes_to_read, max_bytes_per_op);

    static constexpr size_t max_prdt_entries = (Page::size - CommandTable::size) / PRDTEntry::size;

    auto* ranges_begin = ranges.data();
    size_t ranges_count = ranges.size();

    List<OP> ops;

    OP operation {};
    operation.type = op;
    operation.port = port;
    operation.is_async = is_async;

    while (ranges_count) {
        size_t bytes_for_this_op = 0;

        while (bytes_for_this_op < max_bytes_per_op && ranges_begin != ranges.end() && operation.physical_ranges.size() < max_prdt_entries) {
            if (bytes_for_this_op + ranges_begin->length() > max_bytes_per_op)
                break;

            bytes_for_this_op += ranges_begin->length();
            operation.physical_ranges.append(*ranges_begin);
            ranges_begin++;
        }

        if (bytes_for_this_op == 0)
            break;

        ranges_count -= operation.physical_ranges.size();

        ASSERT((bytes_for_this_op % state.logical_sector_size) == 0);
        bytes_for_this_op = min(bytes_for_this_op, state.logical_sector_size * lba_range.length());
        auto lba_count_for_this_batch = bytes_for_this_op / state.logical_sector_size;
        bytes_to_read -= bytes_for_this_op;

        operation.lba_range.set_begin(lba_range.begin());
        operation.lba_range.set_length(lba_count_for_this_batch);

        lba_range.advance_begin_by(lba_count_for_this_batch);

        ops.append_back(operation);
        operation.physical_ranges.clear();
    }

    return ops;
}

void AHCI::execute(const OP& op)
{
    AHCI_DEBUG << "executing op " << op.type_to_string() << " | lba range: "
               << op.lba_range << " | " << op.physical_ranges.size() << " physical range(s) | slot " << op.command_slot;

    auto& port = m_ports[op.port];
    auto& ch = port.command_list->commands[op.command_slot];

    ch.physical_region_descriptor_table_length = op.physical_ranges.size();
    ch.write = static_cast<u8>(op.type);
    ch.command_fis_length = FISHostToDevice::size_in_dwords;
    ch.atapi = false;
    ch.port_multiplier_port = 0;

    Address command_table_physical = ADDRESS_FROM_TWO_DWORDS(ch.command_table_base_address, ch.command_table_base_address_upper);
    auto command_table = TypedMapping<CommandTable>::create("AHCI command table"_sv, command_table_physical, Page::size);

    FISHostToDevice h2d {};

    ATACommand cmd;
    if (op.type == OP::Type::READ)
        cmd = port.supports_48bit_lba ? ATACommand::READ_DMA_EXT : ATACommand::READ_DMA;
    else if (op.type == OP::Type::WRITE)
        cmd = port.supports_48bit_lba ? ATACommand::WRITE_DMA_EXT : ATACommand::WRITE_DMA;
    else
        ASSERT_NEVER_REACHED();

    h2d.command_register = cmd;
    h2d.is_command = true;

    h2d.count_lower = op.lba_range.length() & 0xFF;
    h2d.count_upper = (op.lba_range.length() >> 8) & 0xFF;

    static constexpr u8 lba_bit = SET_BIT(6);
    h2d.device_register = lba_bit;

    h2d.lba_1 = (op.lba_range.begin() >> 0) & 0xFF;
    h2d.lba_2 = (op.lba_range.begin() >> 8) & 0xFF;
    h2d.lba_3 = (op.lba_range.begin() >> 16) & 0xFF;
    h2d.lba_4 = (op.lba_range.begin() >> 24) & 0xFF;
    h2d.lba_5 = (op.lba_range.begin() >> 32) & 0xFF;
    h2d.lba_6 = (op.lba_range.begin() >> 40) & 0xFF;

    copy_memory(&h2d, &command_table->command_fis, h2d.size_in_dwords * sizeof(u32));

    size_t prdt_entry_index = 0;
    size_t bytes_to_read = op.lba_range.length() * port.logical_sector_size;

    for (auto& phys_range : op.physical_ranges) {
        auto& prdt_entry = command_table->prdt_entries[prdt_entry_index++];

        prdt_entry.interrupt_on_completion = false;
        prdt_entry.byte_count = min(bytes_to_read, phys_range.length()) - 1;
        bytes_to_read -= prdt_entry.byte_count;

        SET_DWORDS_TO_ADDRESS(prdt_entry.data_base, prdt_entry.data_upper, phys_range.begin());
    }

    if (!op.is_async)
        synchronous_wait_for_command_completion(op.port, op.command_slot);
    else
        m_hba->ports[op.port].command_issue = SET_BIT(op.command_slot);
}

void AHCI::synchronous_wait_for_command_completion(size_t port, size_t slot)
{
    auto command_bit = SET_BIT(slot);

    m_hba->ports[port].command_issue = command_bit;

    static constexpr size_t max_wait_timeout = Time::nanoseconds_in_millisecond * 500;
    auto wait_begin = Timer::nanoseconds_since_boot();
    auto wait_end = wait_begin + max_wait_timeout;

    while (m_hba->ports[port].command_issue & command_bit) {
        if (wait_end < Timer::nanoseconds_since_boot())
            break;
    }

    if (m_hba->ports[port].command_issue & command_bit)
        runtime::panic("AHCI: command failed to complete within 500ms");
}

void AHCI::panic_if_port_error(PortInterruptStatus status, PortSATAError error)
{
    if (!status.contains_errors() && !error.contains_errors())
        return;

    String error_string;
    error_string << "Encountered an unexpected port error:\nStatus: "
                 << status.error_string() << "\nSATA Error: " << error.error_string();

    runtime::panic(error_string.c_string());
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
    // We must enable this so we can receive the port signature
    auto cmd = port_read<PortCommandAndStatus>(index);
    cmd.fis_receive_enable = true;
    port_write(index, cmd);

    auto sctl = port_read<PortSATAControl>(index);
    sctl.device_detection_initialization = PortSATAControl::DeviceDetectionInitialization::PERFORM_INITIALIZATION;
    port_write(index, sctl);

    static constexpr size_t comreset_delivery_wait = Time::nanoseconds_in_millisecond;
    auto wait_begin = Timer::nanoseconds_since_boot();
    auto wait_end = wait_begin + comreset_delivery_wait;

    while (wait_end > Timer::nanoseconds_since_boot())
        ;

    sctl = port_read<PortSATAControl>(index);
    sctl.device_detection_initialization = PortSATAControl::DeviceDetectionInitialization::NOT_REQUESTED;
    port_write(index, sctl);

    wait_begin = Timer::nanoseconds_since_boot();
    wait_end = wait_begin + phy_wait_timeout;

    auto ssts = port_read<PortSATAStatus>(index);

    while (ssts.device_detection != PortSATAStatus::DeviceDetection::DEVICE_PRESENT_PHY) {
        ssts = port_read<PortSATAStatus>(index);

        if (wait_end < Timer::nanoseconds_since_boot())
            break;
    }

    if (ssts.device_detection != PortSATAStatus::DeviceDetection::DEVICE_PRESENT_PHY)
        runtime::panic("AHCI: Port physical layer failed to come back online after reset");

    m_hba->ports[index].error = 0xFFFFFFFF;

    wait_for_port_ready(index);

    m_hba->ports[index].error = 0xFFFFFFFF;

    AHCI_LOG << "successfully reset port " << index;
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
    auto pi = m_hba->ports_implemented;
    auto fis_receive_page = allocate_safe_page();
    auto command_list_page = allocate_safe_page();

    auto fis_res = TypedMapping<u8>::create("HBA RESET"_sv, fis_receive_page, Page::size);

    for (size_t i = 0; i < 32; ++i) {
        if (!IS_BIT_SET(pi, i))
            continue;

        auto& port = m_hba->ports[i];
        SET_DWORDS_TO_ADDRESS(port.fis_base_address, port.fis_base_address_upper, fis_receive_page);
        SET_DWORDS_TO_ADDRESS(port.command_list_base_address, port.command_list_base_address_upper, command_list_page);

        auto cas = port_read<PortCommandAndStatus>(i);
        cas.fis_receive_enable = true;
        port_write(i, cas);

        if (should_spinup) {
            auto cas = port_read<PortCommandAndStatus>(i);
            cas.spinup_device = true;
            port_write(i, cas);
        }

        auto wait_begin = Timer::nanoseconds_since_boot();
        auto wait_end = wait_begin + phy_wait_timeout;

        auto ssts = port_read<PortSATAStatus>(i);

        // Even though the spec says to wait for either 1h or 3h, it is wrong.
        // If we were to break on 1h we would clear the error register too early and the port
        // would hang on PxTFD.STS.BSY
        // The reason for that is because each port goes from 0 to 1 to 3 gradually as Phy
        // comes back online, so we have to wait for exactly 3 and only clear the error register
        // after that.
        while (ssts.device_detection != PortSATAStatus::DeviceDetection::DEVICE_PRESENT_PHY) {
            ssts = port_read<PortSATAStatus>(i);

            if (wait_end < Timer::nanoseconds_since_boot())
                break;
        }

        if (ssts.device_detection != PortSATAStatus::DeviceDetection::DEVICE_PRESENT_PHY)
            continue;

        m_hba->ports[i].error = 0xFFFFFFFF;

        wait_for_port_ready(i);

        m_hba->ports[i].error = 0xFFFFFFFF;

        if (i != 31) {
            // Zero each time so each port starts at zeroed FIS receive
            zero_memory(fis_res.get(), Page::size);
        }
    }

    MemoryManager::the().free_page(Page(fis_receive_page));
    MemoryManager::the().free_page(Page(command_list_page));

    AHCI_LOG << "succesfully reset HBA";
}

void AHCI::wait_for_port_ready(size_t index)
{
    static constexpr size_t port_reset_timeout = 1 * Time::nanoseconds_in_second;
    auto wait_start_ts = Timer::nanoseconds_since_boot();
    auto wait_end_ts = wait_start_ts + port_reset_timeout;

    auto tfd = port_read<PortTaskFileData>(index);
    while (tfd.status.busy || tfd.status.drq) {
        if (Timer::nanoseconds_since_boot() > wait_end_ts)
            break;

        tfd = port_read<PortTaskFileData>(index);
    }

    if (tfd.status.busy || tfd.status.drq) {
        String error_str;
        error_str << "AHCI: port " << index << " reset failed - PxTFD.STS.BSY: "
                  << format::as_hex << tfd.status.busy << " PxTFD.STS.DRQ: "
                  << tfd.status.drq << " PxTFD.STS.ERR: " << tfd.status.error;

        runtime::panic(error_str.c_string());
    }
}

void AHCI::perform_bios_handoff()
{
    if (hba_read<CAP2>().bios_handoff == false)
        return;

    AHCI_LOG << "Performing BIOS handoff...";

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
    // Cannot use MSIs
    if (InterruptController::is_legacy_mode())
        return;

    for (const auto& device : devices) {
        if (class_code != device.class_code)
            continue;
        if (subclass_code != device.subclass_code)
            continue;
        if (programming_interface != device.programming_interface)
            continue;

        AHCI_LOG << "detected a new controller -> " << device;
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

Optional<size_t> AHCI::PortState::allocate_slot()
{
    auto slot = slot_map.find_bit(false);

    if (slot.has_value())
        slot_map.set_bit(slot.value(), true);

    return slot;
}

void AHCI::PortState::deallocate_slot(size_t slot)
{
    slot_map.set_bit(slot, false);
}

StringView AHCI::PortState::type_to_string()
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

}
