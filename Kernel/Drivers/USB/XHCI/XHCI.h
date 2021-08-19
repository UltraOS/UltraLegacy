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

private:
#ifdef ULTRA_32
    MemoryManager::VR m_bar0_region;
#endif

    volatile CapabilityRegisters* m_capability_registers { nullptr };
    volatile OperationalRegisters* m_operational_registers { nullptr };
};

}
