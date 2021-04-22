#include "SVGA-II.h"

#include "Common/Logger.h"
#include "Interrupts/MP.h"
#include "Core/IO.h"

#define SVGAII_LOG info("SVGA-II")
#define SVGAII_WARN warning("SVGA-II")

#define SVGA_DEBUG_MODE

#ifdef SVGA_DEBUG_MODE
#define SVGAII_DEBUG info("SVGA-II")
#else
#define SVGAII_DEBUG DummyLogger()
#endif

namespace kernel {

SVGAII::SVGAII(const PCI::DeviceInfo& info)
    : ::kernel::Device(Category::VIDEO)
    , PCI::Device(info)
    , NonMSIPCIIRQHandler({}, Type::LEGACY, legacy_pci_irq_line())
{
    initialize();
}

SVGAII::SVGAII(const PCI::DeviceInfo& info, const PCIIRQ& routing_info)
    : ::kernel::Device(Category::VIDEO)
    , PCI::Device(info)
    , NonMSIPCIIRQHandler(routing_info, Type::FIXED)
{
    initialize();
}

void SVGAII::initialize()
{
    enable_memory_space();
    make_bus_master();

    auto io_base = bar(0);
    ASSERT(io_base.type == BAR::Type::IO);
    m_io_base = io_base.address;
    SVGAII_DEBUG << "IO base at " << format::as_hex << m_io_base;

    auto fb_base = bar(1);
    ASSERT(fb_base.type == BAR::Type::MEMORY);
    m_fb_base = fb_base.address;
    SVGAII_DEBUG << "Framebuffer base at " << m_fb_base;

    auto fifo_base = bar(2);
    ASSERT(fifo_base.type = BAR::Type::MEMORY);
    m_fifo_base = fifo_base.address;
    SVGAII_DEBUG << "FIFO base at " << m_fifo_base;

    negotiate_version();

    m_fb_size = read(Register::FRAMEBUFFER_SIZE);
    m_fifo_size = read(Register::FIFO_SIZE);

    SVGAII_DEBUG << "FB size: " << (m_fb_size / MB) << "MB, FIFO size: " << (m_fifo_size / KB) << "KB";

    m_caps = bit_cast<Capabilities>(read(Register::CAPABILITIES));

    if (m_caps.capabilities_2_register)
        m_caps_2 = bit_cast<Capabilities2>(read(Register::CAPABILITIES2));

    dump_capabilities();
}

void SVGAII::dump_capabilities()
{
    SVGAII_DEBUG << "Capability: \"RECT_COPY\" present: " << m_caps.rect_copy;
    SVGAII_DEBUG << "Capability: \"CURSOR\" present: " << m_caps.cursor;
    SVGAII_DEBUG << "Capability: \"CURSOR_BYPASS\" present: " << m_caps.cursor_bypass_1;
    SVGAII_DEBUG << "Capability: \"CURSOR_BYPASS_2\" present: " << m_caps.cursor_bypass_2;
    SVGAII_DEBUG << "Capability: \"8BIT_EMULATION\" present: " << m_caps.emulation_8bit;
    SVGAII_DEBUG << "Capability: \"ALPHA_CURSOR\" present: " << m_caps.alpha_cursor;
    SVGAII_DEBUG << "Capability: \"3D\" present: " << m_caps.three_d;
    SVGAII_DEBUG << "Capability: \"EXTENDED_FIFO\" present: " << m_caps.extended_fifo;
    SVGAII_DEBUG << "Capability: \"MULTIMON\" present: " << m_caps.multi_monitor;
    SVGAII_DEBUG << "Capability: \"PITCHLOCK\" present: " << m_caps.pitch_lock;
    SVGAII_DEBUG << "Capability: \"IRQMASK\" present: " << m_caps.irq_mask;
    SVGAII_DEBUG << "Capability: \"DISPLAY_TOPOLOGY\" present: " << m_caps.display_topology;
    SVGAII_DEBUG << "Capability: \"GMR\" present: " << m_caps.guest_memory_regions_1;
    SVGAII_DEBUG << "Capability: \"TRACES\" present: " << m_caps.traces;
    SVGAII_DEBUG << "Capability: \"GMR2\" present: " << m_caps.guest_memory_regions_2;
    SVGAII_DEBUG << "Capability: \"SCREEN_OBJECT_2\" present: " << m_caps.screen_object_2;
    SVGAII_DEBUG << "Capability: \"COMMAND_BUFFERS\" present: "  << m_caps.command_buffers_1;
    SVGAII_DEBUG << "Capability: \"DEAD1\" present: " << m_caps.dead_1;
    SVGAII_DEBUG << "Capability: \"CMD_BUFFERS_2\" present: "  << m_caps.command_buffers_2;
    SVGAII_DEBUG << "Capability: \"GBOBJECTS\" present: "  << m_caps.guest_backed_objects;
    SVGAII_DEBUG << "Capability: \"DX\" present: "  << m_caps.dx;
    SVGAII_DEBUG << "Capability: \"HP_CMD_QUEUE\" present: "  << m_caps.high_priority_command_queue;
    SVGAII_DEBUG << "Capability: \"NO_BB_RESTRICTION\" present: "  << m_caps.no_bounding_box_restrictions;
    SVGAII_DEBUG << "Capability: \"CAP2_REGISTER\" present: "  << m_caps.capabilities_2_register;

    SVGAII_DEBUG << "Capability: \"GROW_OTABLE\" present: " << m_caps_2.grow_otable;
    SVGAII_DEBUG << "Capability: \"INTRA_SURFACE_COPY\" present: " << m_caps_2.intra_surface_copy;
    SVGAII_DEBUG << "Capability: \"DX2\" present: " << m_caps_2.dx2;
    SVGAII_DEBUG << "Capability: \"GB_MEMSIZE_2\" present: " << m_caps_2.gb_memsize_2;
    SVGAII_DEBUG << "Capability: \"SCREENDMA_REG\" present: " << m_caps_2.screen_dma_register;
    SVGAII_DEBUG << "Capability: \"OTABLE_PTDEPTH_2\" present: " << m_caps_2.otable_pdepth_2;
    SVGAII_DEBUG << "Capability: \"NON_MS_TO_MS_STRETCHBLT\" present: " << m_caps_2.non_multisampled_to_multisampled_stretch_blit;
    SVGAII_DEBUG << "Capability: \"CURSOR_MOB\" present: " << m_caps_2.cursor_mobid;
    SVGAII_DEBUG << "Capability: \"MSHINT\" present: " << m_caps_2.mshint;
    SVGAII_DEBUG << "Capability: \"DX3\" present: " << m_caps_2.dx3;
}

void SVGAII::negotiate_version()
{
    static constexpr u32 svga_magic = 0x900000;
    static constexpr u32 svga_magic_offset = 8;
    static constexpr u32 version_0 = 0;
    static constexpr u32 version_1 = 1;
    static constexpr u32 version_2 = 2;

    [[maybe_unused]] static constexpr u32 svga_version_0 = (svga_magic << svga_magic_offset) | version_0;
    [[maybe_unused]] static constexpr u32 svga_version_1 = (svga_magic << svga_magic_offset) | version_1;
    [[maybe_unused]] static constexpr u32 svga_version_2 = (svga_magic << svga_magic_offset) | version_2;

    u32 negotiated_version = svga_version_2;

    do {
        write(Register::ID, negotiated_version);

        if (read(Register::ID) == negotiated_version)
            break;

        negotiated_version--;
    } while (negotiated_version >= svga_version_0);

    if (negotiated_version < svga_version_0)
        runtime::panic("Failed to negotiate SVGA version");

    SVGAII_DEBUG << "negotiated SVGA version " << (negotiated_version & 0xFF);
}

u32 SVGAII::read(Register reg)
{
    IO::out32(m_io_base + base_index_offset, static_cast<u32>(reg));
    return IO::in32(m_io_base + base_value_offset);
}

void SVGAII::write(Register reg, u32 value)
{
    IO::out32(m_io_base + base_index_offset, static_cast<u32>(reg));
    IO::out32(m_io_base + base_value_offset, value);
}

void SVGAII::autodetect(const DynamicArray<PCI::DeviceInfo>& devices)
{
    for (auto& device : devices) {
        if (device.vendor_id != vendor_id || device.device_id != device_id)
            continue;

        SVGAII_LOG << "detected at " << device;

        if (InterruptController::is_legacy_mode()) {
            new SVGAII(device);
            return;
        }

        // We have to try to deduce IOAPIC pin from MP tables
        auto& location = device.location;
        auto routing = MP::try_deduce_pci_irq_number(location.bus, location.device);
        if (!routing) {
            SVGAII_WARN << "Failed to deduce PCI IRQ routing information";
            return;
        }

        SVGAII_DEBUG << "deduced PCI routing information: IOAPIC "
                     << routing->ioapic_id << ", pin " << routing->ioapic_pin;

        new SVGAII(device, *routing);
    }
}

bool SVGAII::handle_irq(RegisterState&)
{
    SVGAII_WARN << "IRQ";
    return true;
}

}