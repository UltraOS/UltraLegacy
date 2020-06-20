#pragma once

#include "Common/Types.h"
#include "Common/Macros.h"
#include "Common/Logger.h"

namespace kernel {
    struct PACKED RegisterState
    {
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
        u32 error_code;
        u32 eip;
        u32 cs;
        u32 eflags;
        u32 userspace_esp;
        u32 userspace_ss;
    };

    class PageFault
    {
    public:
        enum Type : u8
        {
            READ_NON_PRESENT  = 0,
            READ_PROTECTION   = 1,
            WRITE_NON_PRESENT = 2,
            WRITE_PROTECTION  = 3,
        };

        PageFault(ptr_t address, bool is_user, Type type)
            : m_address(address), m_is_user(is_user), m_type(type)
        {
        }

        ptr_t address() const
        {
            return m_address;
        }

        Type type() const
        {
            return m_type;
        }

        bool is_user() const
        {
            return m_is_user;
        }

        bool is_kernel() const
        {
            return !is_user();
        }

        static const char* type_as_string(Type t)
        {
            switch (t)
            {
                case READ_NON_PRESENT:  return "non-present page read";
                case READ_PROTECTION:   return "read a non-readable page";
                case WRITE_NON_PRESENT: return "non-present page write";
                case WRITE_PROTECTION:  return "write a non-writable page";
            }

            return "unknown";
        }

        template<typename LoggerT>
        friend LoggerT& operator<<(LoggerT&& logger, const PageFault& fault)
        {
            logger << format::as_hex
                   << "\n------> Address: " << fault.address()
                   << "\n------> Type   : " << type_as_string(fault.type())
                   << "\n------> Is user: " << fault.is_user();

            return logger;
        }

    private:
        ptr_t m_address { 0 };
        bool  m_is_user { 0 };
        Type  m_type    { READ_NON_PRESENT };
    };
}
