#pragma once

#include "Drivers/Device.h"
#include "Drivers/PCI/PCI.h"
#include "Interrupts/IRQHandler.h"

namespace kernel {

class SVGAII : public Device, public PCI::Device, public NonMSIPCIIRQHandler {
    AUTO_DETECT_PCI(SVGAII)
public:
    static constexpr u16 vendor_id = 0x15AD;
    static constexpr u16 device_id = 0x0405;

    SVGAII(const PCI::DeviceInfo&);
    SVGAII(const PCI::DeviceInfo&, const PCIIRQ&);

    virtual StringView device_type() const override { return "VMWare Graphics Adapter"_sv; }
    virtual StringView device_model() const override { return "SVGA-II"_sv; }

    static void autodetect(const DynamicArray<PCI::DeviceInfo>&);

private:
    void initialize();
    void negotiate_version();

    bool handle_irq(RegisterState&) override;

    enum class Register : u32 {
        ID = 0,
        FRAMEBUFFER_SIZE = 16,
        CAPABILITIES = 17,
        FIFO_SIZE = 19,
        CAPABILITIES2 = 59,
    };

    static constexpr u16 base_index_offset = 0;
    static constexpr u16 base_value_offset = 1;
    u32 read(Register);
    void write(Register, u32);

    struct PACKED Capabilities {
        u8 reserved_1 : 1;
        bool rect_copy : 1;
        u8 reserved_2 : 3;
        bool cursor : 1;
        bool cursor_bypass_1 : 1;
        bool cursor_bypass_2 : 1;
        bool emulation_8bit : 1;
        bool alpha_cursor : 1;
        u8 reserved_3 : 4;
        bool three_d : 1;
        bool extended_fifo : 1;
        bool multi_monitor : 1;
        bool pitch_lock : 1;
        bool irq_mask : 1;
        bool display_topology : 1;
        bool guest_memory_regions_1 : 1;
        bool traces : 1;
        bool guest_memory_regions_2 : 1;
        bool screen_object_2 : 1;
        bool command_buffers_1 : 1;
        bool dead_1 : 1;
        bool command_buffers_2 : 1;
        bool guest_backed_objects : 1;
        bool dx : 1;
        bool high_priority_command_queue : 1;
        bool no_bounding_box_restrictions : 1;
        bool capabilities_2_register : 1;

        static constexpr size_t size = sizeof(u32);
    };

    static_assert(sizeof(Capabilities) == Capabilities::size, "Invalid size of Capabilities");

    struct PACKED Capabilities2 {
        bool grow_otable : 1;
        bool intra_surface_copy : 1;
        bool dx2 : 1;
        bool gb_memsize_2 : 1;
        bool screen_dma_register : 1;
        bool otable_pdepth_2 : 1;
        bool non_multisampled_to_multisampled_stretch_blit : 1;
        bool cursor_mobid : 1;
        bool mshint : 1;
        bool dx3 : 1;
        u32 reserved : 22;

        static constexpr size_t size = sizeof(u32);
    };

    static_assert(sizeof(Capabilities2) == Capabilities2::size, "Invalid size of Capabilities2");

    void dump_capabilities();

private:
    u16 m_io_base;

    Address m_fb_base;
    size_t m_fb_size;

    Address m_fifo_base;
    size_t m_fifo_size;

    Capabilities m_caps { };
    Capabilities2 m_caps_2 { };
};

}