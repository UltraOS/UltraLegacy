#pragma once

#include "Kernel/Drivers/Device.h"
#include "Kernel/Drivers/PCI/PCI.h"
#include "Kernel/Interrupts/IRQHandler.h"
#include "Kernel/Memory/MemoryManager.h"

#include "Structures.h"

namespace kernel {

class XHCI final : public Device, public PCI::Device, public IRQHandler {
    AUTO_DETECT_PCI(XHCI);

public:
    explicit XHCI(const PCI::DeviceInfo&);

    static constexpr u8 class_code = 0x0C;
    static constexpr u8 subclass_code = 0x03;
    static constexpr u8 programming_interface = 0x30;

    StringView device_type() const override { return "xHCI"_sv; }
    StringView device_model() const override { return "Generic xHCI Controller"_sv; }

    bool handle_irq(RegisterState&) override;

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

    bool perform_bios_handoff();
    bool halt();
    bool reset();

    template <typename T>
    static T read_reg(Address);

    template<typename T>
    static void write_reg(T, Address);

    bool detect_ports();
    Address find_extended_capability(u8, Address only_after = nullptr);

private:
#ifdef ULTRA_32
    MemoryManager::VR m_bar0_region;
#endif

    volatile CapabilityRegisters* m_capability_registers { nullptr };
    volatile OperationalRegisters* m_operational_registers { nullptr };
    Address m_ecaps_base { nullptr };

    struct Port {
        enum Mode : u8 {
            NOT_PRESENT = 0,
            USB2 = SET_BIT(1),
            USB2_MASTER = SET_BIT(1) | SET_BIT(0),
            USB3 = SET_BIT(2),
            USB3_MASTER = SET_BIT(2) | SET_BIT(0)
        } mode { NOT_PRESENT };

        u8 index_of_pair { 0 };
        u8 physical_offset { 0 };

        [[nodiscard]] bool is_master() const { return mode & SET_BIT(0); }

        StringView mode_to_string()
        {
            switch (mode) {
            case NOT_PRESENT:
                return "not present"_sv;
            case USB2:
                return "USB2"_sv;
            case USB2_MASTER:
                return "USB2 master"_sv;
            case USB3:
                return "USB3"_sv;
            case USB3_MASTER:
                return "USB3 master"_sv;
            default:
                return "invalid"_sv;
            }
        }
    }* m_ports { nullptr };
    size_t m_port_count { 0 };
};

}
