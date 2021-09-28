#pragma once

#include "Kernel/Drivers/Device.h"
#include "Kernel/Drivers/PCI/PCI.h"
#include "Kernel/Interrupts/IRQHandler.h"
#include "Kernel/Interrupts/DeferredIRQ.h"
#include "Kernel/Memory/MemoryManager.h"
#include "Memory/TypedMapping.h"

#include "Structures.h"

namespace kernel {

class XHCI final : public Device, public PCI::Device, public IRQHandler, public DeferredIRQHandler {
    AUTO_DETECT_PCI(XHCI);

public:
    explicit XHCI(const PCI::DeviceInfo&);

    static constexpr u8 class_code = 0x0C;
    static constexpr u8 subclass_code = 0x03;
    static constexpr u8 programming_interface = 0x30;

    StringView device_type() const override { return "xHCI"_sv; }
    StringView device_model() const override { return "Generic xHCI Controller"_sv; }

    bool handle_irq(RegisterState&) override;
    bool handle_deferred_irq() override;

    static void autodetect(const DynamicArray<PCI::DeviceInfo>&);

private:
    bool initialize();

    template <typename T>
    T read_cap_reg();

    template <typename T>
    void write_cap_reg(T);

    template <typename T>
    T read_oper_reg();

    template <typename T>
    void write_oper_reg(T);

    template <typename T>
    T read_interrupter_reg(size_t index);

    template <typename T>
    void write_interrupter_reg(size_t index, T value);

    template <typename T>
    void write_port_reg(size_t index, T value);

    template <typename T>
    T read_port_reg(size_t index);

    // NOTE: You're not required to set the cycle bit.
    template <typename T>
    size_t enqueue_command(T&);

    void ring_doorbell(size_t index, DoorbellRegister);

    bool perform_bios_handoff();
    bool initialize_dcbaa(const HCCPARAMS1&);
    bool initialize_command_ring();
    bool initialize_event_ring();

    bool halt();
    bool reset();

    bool detect_ports();
    Address find_extended_capability(u8, Address only_after = nullptr);

    Page allocate_safe_page();

    void reset_port(size_t port_id, bool warm);
    void handle_port_status_change(size_t port_id);
    void handle_port_connect_status_change(size_t port_id, PORTSC status);
    void handle_port_reset_status_change(size_t port_id, PORTSC status);
    void handle_port_config_error(size_t port_id, PORTSC status);

    void begin_port_initialization(size_t port_id);

private:
#ifdef ULTRA_32
    MemoryManager::VR m_bar0_region;
#endif

    volatile CapabilityRegisters* m_capability_registers { nullptr };
    volatile OperationalRegisters* m_operational_registers { nullptr };
    volatile RuntimeRegisters* m_runtime_registers { nullptr };
    volatile u32* m_doorbell_registers { nullptr };
    Address m_ecaps_base { nullptr };

    struct Port {
        enum Mode : u8 {
            NOT_PRESENT = 0,
            USB2 = SET_BIT(1),
            USB2_PAIRED = SET_BIT(1) | SET_BIT(0),
            USB3 = SET_BIT(2),
            USB3_PAIRED = SET_BIT(2) | SET_BIT(0)
        } mode { NOT_PRESENT };

        enum class State : u8 {
            IDLE,
            SPURIOUS_CONNECTION,
            RESETTING,
            INITIALIZATION,
            RUNNING,
        } state { State::IDLE };


        u8 index_of_pair { 0 };
        u8 physical_offset { 0 };

        [[nodiscard]] bool is_paired() const { return mode & SET_BIT(0); }
        [[nodiscard]] bool is_usb2() const { return mode & USB2; }
        [[nodiscard]] bool is_usb3() const { return mode & USB3; }

        [[nodiscard]] StringView mode_to_string() const;
        [[nodiscard]] StringView state_to_string() const;

        void set_index_of_pair(size_t index)
        {
            index_of_pair = index;
            mode = static_cast<Mode>(static_cast<u8>(mode) | SET_BIT(0));
        }
    } * m_ports { nullptr };
    size_t m_port_count { 0 };

    struct DCBAAContext {
        TypedMapping<u64> dcbaa;
        Page scratchpad_buffer_array_page;
        DynamicArray<Page> scratchpad_pages;

        size_t bytes_per_context_structure { 0 };
    } m_dcbaa_context;

    static constexpr size_t command_ring_capacity = Page::size / sizeof(GenericTRB);
    size_t m_command_ring_offset { 0 };
    Page m_command_ring_page;
    TypedMapping<volatile GenericTRB> m_command_ring;

    static constexpr size_t event_ring_segment_capacity = Page::size / sizeof(GenericTRB);
    Page m_event_ring_segment0_page {};
    size_t m_event_ring_index { 0 };
    TypedMapping<volatile GenericTRB> m_event_ring;

    u8 m_event_ring_cycle { 1 };
    u8 m_command_ring_cycle { 1 };

    bool m_supports_64bit { false };
};

}
