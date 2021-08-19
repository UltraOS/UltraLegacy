#include "XHCI.h"
#include "Structures.h"

#include "Common/Logger.h"

#define XHCI_LOG log("XHCI")
#define XHCI_WARN warning("XHCI")

#define XHCI_DEBUG_MODE

#ifdef XHCI_DEBUG_MODE
#define XHCI_DEBUG log("XHCI")
#else
#define XHCI_DEBUG DummyLogger()
#endif

namespace kernel {

struct WaitResult {
    bool success;
    size_t elapsed_ms;
};

template <typename Callback>
static WaitResult repeat_until(Callback cb, size_t timeout_ms)
{
    u64 wait_begin = Timer::nanoseconds_since_boot();
    u64 current_time = wait_begin;
    u64 wait_end = current_time + timeout_ms * Time::nanoseconds_in_millisecond;

    while (current_time <= wait_end)
    {
        if (cb())
            return { true, static_cast<size_t>((current_time - wait_begin) / Time::nanoseconds_in_millisecond) };

        current_time = Timer::nanoseconds_since_boot();
    }

    return { cb(), timeout_ms };
}

XHCI::XHCI(const PCI::DeviceInfo& info)
    : ::kernel::Device(Category::CONTROLLER)
    , PCI::Device(info)
    , IRQHandler(IRQHandler::Type::MSI)
{
}

bool XHCI::initialize()
{
    auto bar0 = bar(0);
    XHCI_LOG << "BAR0 @ "<< bar0.address << ", spans " << bar0.span / KB << "KB";

    if (!can_use_bar(bar0)) {
        XHCI_WARN << "Cannot utilize BAR0, outside of usable address space. Device skipped.";
        return false;
    }

#ifdef ULTRA_32
    m_bar0_region = MemoryManager::the().allocate_kernel_non_owning("xHCI"_sv, { bar0.address, Page::round_up(bar0.span) });
    m_capability_registers = m_bar0_region->virtual_range().begin().as_pointer<volatile CapabilityRegisters>();
#elif defined(ULTRA_64)
    m_capability_registers = MemoryManager::physical_to_virtual(bar0.address).as_pointer<volatile CapabilityRegisters>();
#endif

    auto caplength = m_capability_registers->CAPLENGTH;
    XHCI_DEBUG << "Operational registers at offset " << caplength;

    auto base = Address(m_capability_registers);
    base += caplength;
    m_operational_registers = base.as_pointer<OperationalRegisters>();

    if (!perform_bios_handoff()) {
        XHCI_WARN << "Failed to acquire controller ownership";
        return false;
    }

    if (!reset())
        return false;

    return true;
}

bool XHCI::perform_bios_handoff()
{
    auto hccp1 = read_cap_reg<HCCPARAMS1>();
    auto caps_offset = hccp1.xECP * sizeof(u32);

    if (!caps_offset) {
        XHCI_WARN << "xECP offset is 0";
        return false;
    }

    XHCI_DEBUG << "xECP at offset " << hccp1.xECP;

    static constexpr u32 legacy_support_id = 1;

    Address reg_offset = m_capability_registers;
    reg_offset += caps_offset;
    auto raw = *reg_offset.as_pointer<volatile u32>();

    size_t i = 0;

    while ((raw & 0xFF) != legacy_support_id) {
        log() << "detected id [" << i++ << "] " << (raw & 0xFF);

        auto next_offset = (raw & 0xFF00) >> 8;
        next_offset *= sizeof(u32);

        if (next_offset == 0)
            break;

        reg_offset += next_offset;
        raw = *reg_offset.as_pointer<volatile u32>();
    }

    log() << "detected id ["  << i << "] " << (raw & 0xFF);

    if ((raw & 0xFF) != legacy_support_id) {
        XHCI_DEBUG << "No legacy BIOS support capability detected";
        return true;
    }

    static constexpr u32 os_owned_bit = SET_BIT(24);
    raw |= os_owned_bit;
    *reg_offset.as_pointer<volatile u32>() = raw;

    auto res = repeat_until(
            [reg_offset] () -> bool
            {
                static constexpr u32 bios_owned_bit = SET_BIT(16);
                return (*reg_offset.as_pointer<volatile u32>() & bios_owned_bit) == 0;
            }, 1000);

    if (res.success)
        XHCI_DEBUG << "Successfully performed BIOS handoff";

    return res.success;
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

    u32 raw = *base.as_pointer<volatile u32>();

    return bit_cast<T>(raw);
}

template <typename T>
void XHCI::write_cap_reg(T value)
{
    auto base = Address(m_capability_registers);
    base += T::offset;

    u32 raw = bit_cast<u32>(value);
    *base.as_pointer<volatile u32>() = raw;
}

template <typename T>
T XHCI::read_oper_reg()
{
    auto base = Address(m_operational_registers);
    base += T::offset;

    if constexpr (sizeof(T) == 8) {
#ifdef ULTRA_32
        volatile u32 reg_as_u32[2];
        reg_as_u32[0] = *base.as_pointer<volatile u32>();
        base += sizeof(u32);
        reg_as_u32[1] = *base.as_pointer<volatile u32>();
        return bit_cast<T>(reg_as_u32);
#elif defined(ULTRA_64)
        auto value = *base.as_pointer<volatile u64>();
        return bit_cast<T>(value);
#endif
    } else {
        u32 value = *base.as_pointer<volatile u32>();
        return bit_cast<T>(value);
    }
}

template <typename T>
void XHCI::write_oper_reg(T value)
{
    auto base = Address(m_operational_registers);
    base += T::offset;

    if constexpr (sizeof(T) == 8) {
#ifdef ULTRA_32
        u32 reg_as_u32[2];
        copy_memory(&value, reg_as_u32, 2 * sizeof(u32));
        *base.as_pointer<volatile u32>() = reg_as_u32[0];
        base += sizeof(u32);
        *base.as_pointer<volatile u32>() = reg_as_u32[1];
#elif defined(ULTRA_64)
        auto raw = bit_cast<u64>(value);
        *base.as_pointer<volatile u64>() = raw;
#endif
    } else {
        auto raw = bit_cast<u32>(value);
        *base.as_pointer<volatile u32>() = raw;
    }
}

bool XHCI::handle_irq(RegisterState&)
{
    XHCI_LOG << "got an irq?";
    return true;
}

}
