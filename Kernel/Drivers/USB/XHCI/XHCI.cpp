#include "XHCI.h"
#include "Structures.h"
#include "Drivers/MMIOHelpers.h"

#include "Common/Logger.h"
#include "Core/RepeatUntil.h"

#define XHCI_LOG log("XHCI")
#define XHCI_WARN warning("XHCI")

#define XHCI_DEBUG_MODE

#ifdef XHCI_DEBUG_MODE
#define XHCI_DEBUG log("XHCI")
#else
#define XHCI_DEBUG DummyLogger()
#endif

namespace kernel {

XHCI::XHCI(const PCI::DeviceInfo& info)
    : ::kernel::Device(Category::CONTROLLER)
    , PCI::Device(info)
    , IRQHandler(IRQHandler::Type::MSI)
{
}

bool XHCI::initialize()
{
    auto bar0 = bar(0);
    ASSERT(bar0.is_memory());

    XHCI_LOG << "BAR0 @ "<< bar0.address << ", spans " << bar0.span / KB << "KB";

    if (!can_use_bar(bar0)) {
        XHCI_WARN << "cannot utilize BAR0, outside of usable address space. Device skipped.";
        return false;
    }

#ifdef ULTRA_32
    // Looks like xHCI spec guarantees page alignemnt
    ASSERT(Page::is_aligned(bar0.address));
    m_bar0_region = MemoryManager::the().allocate_kernel_non_owning("xHCI"_sv, { bar0.address, Page::round_up(bar0.span) });
    m_capability_registers = m_bar0_region->virtual_range().begin().as_pointer<volatile CapabilityRegisters>();
#elif defined(ULTRA_64)
    m_capability_registers = MemoryManager::physical_to_virtual(bar0.address).as_pointer<volatile CapabilityRegisters>();
#endif

    auto caplength = m_capability_registers->CAPLENGTH;
    XHCI_DEBUG << "operational registers at offset " << caplength;

    auto base = Address(m_capability_registers);
    base += caplength;
    m_operational_registers = base.as_pointer<OperationalRegisters>();

    auto hccp1 = read_cap_reg<HCCPARAMS1>();

    m_supports_64bit = hccp1.AC64;
    XHCI_DEBUG << "implements 64 bit addressing: " << m_supports_64bit;

    auto ecaps_offset = hccp1.xECP * sizeof(u32);
    m_ecaps_base = m_capability_registers;
    m_ecaps_base += ecaps_offset;

    if (!ecaps_offset) {
        XHCI_WARN << "xECP offset is 0, cannot use controller";
        return false;
    }

    XHCI_DEBUG << "xECP at offset " << hccp1.xECP;

    if (!perform_bios_handoff()) {
        XHCI_WARN << "failed to acquire controller ownership";
        return false;
    }

    auto version = m_capability_registers->HCIVERSION;
    u8 major = (version & 0xFF00) >> 8;
    u8 minor = version & 0x00FF;
    XHCI_LOG << "interface version " << major << "." << minor;

    auto page_size = m_operational_registers->PAGESIZE;
    page_size &= 0xFFFF;
    page_size <<= 12;

    if (page_size != Page::size) {
        XHCI_WARN << "controller has a weird PAGESIZE of " << page_size << ", skipping";
        return false;
    }

    auto hcs = read_cap_reg<HCSPARAMS1>();
    XHCI_DEBUG << "slots: " << hcs.MaxSlots << " interrupters: " << hcs.MaxIntrs << " ports: " << hcs.MaxPorts;
    m_port_count = hcs.MaxPorts;
    m_ports = new Port[m_port_count]{};

    if (!reset())
        return false;

    if (!detect_ports())
        return false;

    auto dcbaap_page = allocate_safe_page();
    m_dcbaa_context.dcbaa = TypedMapping<u64>::create("DCBAA"_sv, dcbaap_page.address(), Page::size);

    auto hcsp2 = read_cap_reg<HCSPARAMS2>();
    auto sb = hcsp2.max_scratchpad_bufs();
    XHCI_DEBUG << "controller asked for " << sb << " scratchpad pages";
    if (sb > (Page::size / sizeof(u64))) {
        XHCI_WARN << "controller requested too many pages, count crosses PAGESIZE boundary(?)";
        return false;
    }

    if (sb) {
        m_dcbaa_context.scratchpad_buffer_array_page = allocate_safe_page();
        m_dcbaa_context.dcbaa[0] = m_dcbaa_context.scratchpad_buffer_array_page.address();

        auto buffer = TypedMapping<u64>::create("Scratchpad", m_dcbaa_context.scratchpad_buffer_array_page.address(), Page::size);

        m_dcbaa_context.scratchpad_pages.reserve(sb);
        for (size_t i = 0; i < sb; ++i) {
            auto scratchpad_page = allocate_safe_page();
            m_dcbaa_context.scratchpad_pages.emplace(scratchpad_page);
            buffer[i] = scratchpad_page.address().raw();
        }
    }

    DCBAAP reg {};
    reg.DeviceContextBaseAddressArrayPointerLo = dcbaap_page.address() & 0xFFFFFFFF;
#ifdef ULTRA_64
    reg.DeviceContextBaseAddressArrayPointerHi = (dcbaap_page.address() >> 32) & 0xFFFFFFFF;
#endif

    m_dcbaa_context.bytes_per_context_structure = hccp1.CSZ ? 64 : 32;
    XHCI_LOG << "bytes per context: " << m_dcbaa_context.bytes_per_context_structure;

    static constexpr size_t entries_per_device_context = 32;
    size_t bytes_per_device_context = entries_per_device_context * m_dcbaa_context.bytes_per_context_structure;
    size_t contexts_per_page = Page::size / bytes_per_device_context;

    XHCI_LOG << bytes_per_device_context << " bytes per device context, " << contexts_per_page << " contexts per page";

    auto pages_needed = ceiling_divide<size_t>(hcs.MaxSlots, contexts_per_page);
    XHCI_LOG << "allocating " << pages_needed << " pages for " << hcs.MaxSlots << " slots";

    m_dcbaa_context.device_context_pages.reserve(pages_needed);
    size_t port_offset = 1;
    size_t contexts_to_initialize = m_port_count;

    for (size_t i = 0; i < pages_needed; ++i) {
        auto page = allocate_safe_page();

        auto current_base = page.address();
        for (size_t j = 0; j < min<size_t>(contexts_to_initialize, contexts_per_page); ++j) {
            m_dcbaa_context.dcbaa[port_offset++] = current_base;
            current_base += bytes_per_device_context;
        }

        contexts_to_initialize -= min<size_t>(contexts_to_initialize, contexts_per_page);

        m_dcbaa_context.device_context_pages.emplace(page);
    }

    write_oper_reg(reg);

    auto command_ring_page = allocate_safe_page();
    m_command_ring = TypedMapping<GenericCommandTRB>::create("CommandRing"_sv, command_ring_page.address(), Page::size);

    // link last TRB back to the head
    LinkTRB link {};
    link.Type = TRBType::Link;
    set_dwords_to_address(link.RingSegmentPointerLo, link.RingSegmentPointerHi, command_ring_page.address());
    copy_memory(&link, &m_command_ring[command_ring_capacity - 1], sizeof(link));

    CRCR cr {};
    cr.RCS = 1;
    cr.CommandRingPointerLo = link.RingSegmentPointerLo >> 6;
    cr.CommandRingPointerHi = link.RingSegmentPointerHi;
    write_oper_reg(cr);

    auto cfg = read_oper_reg<CONFIG>();
    cfg.MaxSlotsEn = hcs.MaxSlots;
    write_oper_reg(cfg);

    auto raw = m_operational_registers->DNCTRL;
    raw |= SET_BIT(1);
    m_operational_registers->DNCTRL = raw;

    // enable the schedule
    auto usbcmd = read_oper_reg<USBCMD>();
    usbcmd.RS = 1;
    usbcmd.INTE = 1;
    usbcmd.HSEE = 1;
    write_oper_reg(usbcmd);

    return true;
}

bool XHCI::detect_ports()
{
    static constexpr u8 supported_protocol_capability_id = 2;
    auto proto_base = find_extended_capability(supported_protocol_capability_id);

    if (!proto_base) {
        XHCI_WARN << "no supported protocol capabilities found, cannot detect ports";
        return false;
    }

    for (; proto_base; proto_base = find_extended_capability(supported_protocol_capability_id, proto_base)) {
        auto dw0 = mmio_read32<SupportedProtocolDWORD0>(proto_base);
        auto dw1 = mmio_read32<SupportedProtocolDWORD1>(proto_base + sizeof(u32));
        auto dw2 = mmio_read32<SupportedProtocolDWORD2>(proto_base + sizeof(u32) * 2);

        if (dw1.NameString != "USB "_sv) {
            XHCI_WARN << "invalid protocol name string " << dw1.NameString;
            return false;
        }

        XHCI_DEBUG << "ports " << dw2.CompatiblePortOffset << " -> " << (dw2.CompatiblePortOffset + dw2.CompatiblePortCount)
                   << " protocol " << dw0.MajorRevision << "." << dw0.MinorRevision;

        for (size_t i = 0; i < dw2.CompatiblePortCount; ++i) {
            auto offset = dw2.CompatiblePortOffset + i - 1;
            if (offset >= m_port_count) {
                XHCI_WARN << "invalid port offset " << offset;
                return false;
            }

            auto& port = m_ports[offset];

            if (dw0.MajorRevision == 3)
                port.mode = Port::USB3_MASTER;
            else if (dw0.MajorRevision == 2)
                port.mode = Port::USB2_MASTER;
            else {
                XHCI_WARN << "invalid revision " << dw0.MajorRevision;
                return false;
            }

            port.physical_offset = i;
        }
    }

    for (size_t i = 0; i < m_port_count; ++i) {
        if (m_ports[i].mode == Port::NOT_PRESENT)
            continue;

        for (size_t j = 0; j < m_port_count; ++j) {
            if (i == j)
                continue;
            if (m_ports[j].mode == Port::NOT_PRESENT)
                continue;

            if (m_ports[i].physical_offset != m_ports[j].physical_offset)
                continue;

            if (m_ports[i].mode == m_ports[j].mode) {
                XHCI_WARN << "port " << i << " and " << j << " are the same but have different offsets?";
                return false;
            }

            m_ports[i].index_of_pair = j;
            m_ports[j].index_of_pair = i;

            if (m_ports[i].mode == Port::USB3_MASTER)
                m_ports[j].mode = Port::USB2;
            if (m_ports[j].mode == Port::USB3_MASTER)
                m_ports[i].mode = Port::USB2;
        }
    }

#ifdef XHCI_DEBUG_MODE
    XHCI_DEBUG << "detected ports and pairings:";

    for (size_t i = 0; i < m_port_count; ++i) {
        auto& port = m_ports[i];

        XHCI_DEBUG << "port[" << i << "] mode: " << port.mode_to_string()
                   << ", offset: " << port.physical_offset << ", pair index: " << port.index_of_pair;
    }
#endif

    return true;
}

Address XHCI::find_extended_capability(u8 id, Address only_after)
{
    auto next_offset_from_cap = [] (Address current) -> Address
    {
        auto raw = mmio_read32<u32>(current);
        auto offset = (raw & 0xFF00) >> 8;
        offset *= sizeof(u32);

        if (offset == 0)
            return nullptr;

        current += offset;
        return current;
    };

    Address search_base = only_after;
    if (search_base)
        search_base = next_offset_from_cap(search_base);
    else
        search_base = m_ecaps_base;

    for (; search_base; search_base = next_offset_from_cap(search_base)) {
        auto raw = mmio_read32<u32>(search_base);

        if ((raw & 0xFF) == id)
            return search_base;
    }

    return search_base;
}

bool XHCI::perform_bios_handoff()
{
    static constexpr u32 legacy_support_id = 1;
    auto legacy_support_base = find_extended_capability(legacy_support_id);

    if (!legacy_support_base) {
        XHCI_DEBUG << "No legacy BIOS support capability detected";
        return true;
    }

    auto raw = mmio_read32<u32>(legacy_support_base);
    static constexpr u32 os_owned_bit = SET_BIT(24);
    raw |= os_owned_bit;
    mmio_write32(legacy_support_base, raw);

    auto res = repeat_until(
            [legacy_support_base] () -> bool
            {
                static constexpr u32 bios_owned_bit = SET_BIT(16);
                return (mmio_read32<u32>(legacy_support_base) & bios_owned_bit) == 0;
            }, 1000);

    if (!res.success)
        return false;

    XHCI_DEBUG << "Successfully performed BIOS handoff";

    // Disable all SMIs because BIOS shouldn't care anymore
    auto legacy_ctl = mmio_read32<USBLEGCTLSTS>(legacy_support_base + sizeof(u32));
    legacy_ctl.USBSMIEnable = 0;
    legacy_ctl.SMIOnHostSystemErrorEnable = 0;
    legacy_ctl.SMIOnOSOwnershipEnable = 0;
    legacy_ctl.SMIOnPCICommandEnable = 0;
    legacy_ctl.SMIOnBAREnable = 0;
    legacy_ctl.SMIOnOSOwnershipChange = 1;
    legacy_ctl.SMIOnPCICommand = 1;
    legacy_ctl.SMIOnBAR = 1;
    mmio_write32(legacy_support_base + sizeof(u32), legacy_ctl);

    return true;
}

bool XHCI::halt()
{
    auto usbcmd = read_oper_reg<USBCMD>();
    usbcmd.RS = 0;
    write_oper_reg(usbcmd);

    auto wait_res = repeat_until([this] () -> bool { return read_oper_reg<USBSTS>().HCH; }, 20);
    if (!wait_res.success)
        XHCI_WARN << "Failed to halt the controller";

    return wait_res.success;
}

bool XHCI::reset()
{
    if (!halt())
        return false;

    auto usbcmd = read_oper_reg<USBCMD>();
    usbcmd.HCRST = 1;
    write_oper_reg(usbcmd);

    auto wait_res = repeat_until(
            [this] () -> bool
            {
                return !read_oper_reg<USBSTS>().CNR && !read_oper_reg<USBCMD>().HCRST;
            }, 100);

    if (!wait_res.success) {
        XHCI_WARN << "Failed to reset the controller";
        return false;
    }

    XHCI_DEBUG << "Controller reset took " << wait_res.elapsed_ms << "MS";
    return true;
}

void XHCI::autodetect(const DynamicArray<PCI::DeviceInfo>& devices)
{
    for (auto& device : devices) {
        if (device.class_code != class_code)
            continue;
        if (device.subclass_code != subclass_code)
            continue;
        if (device.programming_interface != programming_interface)
            continue;

        XHCI_LOG << "detected a new controller -> " << device;
        auto* xhci = new XHCI(device);
        if (xhci->initialize())
            continue;

        delete xhci;
    }
}

template <typename T>
T XHCI::read_cap_reg()
{
    auto base = Address(m_capability_registers);
    base += T::offset;

    return mmio_read32<T>(base);
}

template <typename T>
void XHCI::write_cap_reg(T value)
{
    auto base = Address(m_capability_registers);
    base += T::offset;

    mmio_write32(base, value);
}

template <typename T>
T XHCI::read_oper_reg()
{
    auto base = Address(m_operational_registers);
    base += T::offset;

    if constexpr (sizeof(T) == 8)
        return mmio_read64<T>(base, m_supports_64bit ? MMIOPolicy::QWORD_ACCESS : MMIOPolicy::IGNORE_UPPER_DWORD);
    else
        return mmio_read32<T>(base);
}

template <typename T>
void XHCI::write_oper_reg(T value)
{
    auto base = Address(m_operational_registers);
    base += T::offset;

    if constexpr (sizeof(T) == 8)
        mmio_write64(base, value, m_supports_64bit ? MMIOPolicy::QWORD_ACCESS : MMIOPolicy::IGNORE_UPPER_DWORD);
    else
        mmio_write32(base, value);
}

Page XHCI::allocate_safe_page()
{
    auto page = MemoryManager::the().allocate_page();

#ifdef ULTRA_64
    if (!m_supports_64bit)
        ASSERT(page.address() < 0xFFFFFFFF);
#endif

    return page;
}

bool XHCI::handle_irq(RegisterState&)
{
    XHCI_LOG << "got an irq?";
    return true;
}

}
