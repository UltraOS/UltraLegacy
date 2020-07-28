#pragma once

#include "Common/Logger.h"
#include "Common/Macros.h"
#include "Common/Types.h"
#include "Core/CPU.h"

namespace kernel {

#ifdef ULTRA_32
struct PACKED RegisterState {
    u32 ss;
    u32 gs;
    u32 fs;
    u32 es;
    u32 ds;
    u32 edi;
    u32 esi;
    u32 ebp;
    u32 esp;
    u32 ebx;
    u32 edx;
    u32 ecx;
    u32 eax;

    union {
        u32 irq_number;
        u32 exception_number;
    };

    u32 error_code;
    u32 eip;
    u32 cs;
    u32 eflags;
    u32 userspace_esp;
    u32 userspace_ss;
};
#elif defined(ULTRA_64)
struct PACKED RegisterState {
    u64 r15;
    u64 r14;
    u64 r13;
    u64 r12;
    u64 r11;
    u64 r10;
    u64 r9;
    u64 r8;
    u64 rbp;
    u64 rdi;
    u64 rsi;
    u64 rdx;
    u64 rcx;
    u64 rbx;
    u64 rax;

    union {
        u64 irq_number;
        u64 exception_number;
    };

    u64 error_code;
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
};
#endif

class PageFault {
public:
    enum Type : u8 {
        READ_NON_PRESENT  = 0,
        READ_PROTECTION   = 1,
        WRITE_NON_PRESENT = 2,
        WRITE_PROTECTION  = 3,
    };

    PageFault(Address address, Address instruction_pointer, bool is_user, Type type)
        : m_address(address), m_instruction_pointer(instruction_pointer), m_is_user(is_user), m_type(type)
    {
    }

    Address address() const { return m_address; }
    Address instruction_pointer() const { return m_instruction_pointer; }

    Type type() const { return m_type; }

    bool is_user() const { return m_is_user; }

    bool is_kernel() const { return !is_user(); }

    static StringView type_as_string(Type t)
    {
        switch (t) {
        case READ_NON_PRESENT: return "non-present page read"_sv;
        case READ_PROTECTION: return "read a non-readable page"_sv;
        case WRITE_NON_PRESENT: return "non-present page write"_sv;
        case WRITE_PROTECTION: return "write a non-writable page"_sv;
        }

        return "unknown";
    }

    template <typename LoggerT>
    friend LoggerT& operator<<(LoggerT&& logger, const PageFault& fault)
    {
        logger << "\n------> Address: " << fault.address()
               << "\n------> Instruction at: " << fault.instruction_pointer()
               << "\n------> Type   : " << type_as_string(fault.type()) << "\n------> Is user: " << fault.is_user();

        return logger;
    }

private:
    Address m_address { nullptr };
    Address m_instruction_pointer { nullptr };
    bool    m_is_user { 0 };
    Type    m_type { READ_NON_PRESENT };
};

namespace Interrupts {
    inline bool are_enabled() { return CPU::flags() & CPU::FLAGS::INTERRUPTS; }

    inline void enable() { sti(); }

    inline void disable() { cli(); }

    class ScopedDisabler {
    public:
        ScopedDisabler() : m_should_enable(are_enabled()) { disable(); }

        ~ScopedDisabler()
        {
            if (m_should_enable)
                enable();
        }

    private:
        bool m_should_enable { false };
    };
}
}
