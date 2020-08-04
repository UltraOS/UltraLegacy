#include "Common/Types.h"

namespace kernel {

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

    // clang-format off
    template <typename LoggerT>
    friend LoggerT& operator<<(LoggerT&& logger, const PageFault& fault)
    {
        logger << "\n------> Address: " << fault.address()
               << "\n------> Instruction at: " << fault.instruction_pointer()
               << "\n------> Type   : " << type_as_string(fault.type())
               << "\n------> Is user: " << fault.is_user();

        return logger;
    }
    // clang-format on

private:
    Address m_address { nullptr };
    Address m_instruction_pointer { nullptr };
    bool    m_is_user { 0 };
    Type    m_type { READ_NON_PRESENT };
};
}
