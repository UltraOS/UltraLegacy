#pragma once

#include "Common/String.h"
#include "Common/Types.h"

#include <Shared/Error.h>

namespace kernel {

struct ErrorCode {
#define ERROR_CODE(name, value) name = value,
    enum : ptr_t {
        ENUMERATE_ERROR_CODES
    } value { NO_ERROR };
#undef ERROR_CODE

    using code_t = decltype(value);

    ErrorCode() = default;
    ErrorCode(code_t code)
        : value(code)
    {
    }

    bool is_success() const { return value == NO_ERROR; }
    bool is_error() const { return !is_success(); }

    explicit operator bool() { return is_error(); }

    StringView to_string() const
    {
        switch (value) {
        case NO_ERROR:
            return "No Error"_sv;
        case ACCESS_DENIED:
            return "Access Denied"_sv;
        case INVALID_ARGUMENT:
            return "Invalid Argument"_sv;
        case BAD_PATH:
            return "Bad Path"_sv;
        case DISK_NOT_FOUND:
            return "Disk Not Found"_sv;
        case UNSUPPORTED:
            return "Request Unsupported"_sv;
        case NO_SUCH_FILE:
            return "No Such File"_sv;
        case IS_DIRECTORY:
            return "Is a Directory"_sv;
        case IS_FILE:
            return "Is a File"_sv;
        case FILE_IS_BUSY:
            return "File Is Busy"_sv;
        case FILE_ALREADY_EXISTS:
            return "File Already Exists"_sv;
        case NAME_TOO_LONG:
            return "Name Is Too Long"_sv;
        case BAD_FILENAME:
            return "Filename Is Bad"_sv;
        case INTERRUPTED:
            return "Interrupted"_sv;
        case STREAM_CLOSED:
            return "IO Stream Is Already Closed";
        case WOULD_BLOCK_FOREVER:
            return "Operation Would Block Forever";
        case MEMORY_ACCESS_VIOLATION:
            return "Memory Access Violation";
        default:
            return "<Unknown code>"_sv;
        }
    }
};

template <typename T>
class ErrorOr {
public:
    ErrorOr(ErrorCode code)
        : m_error(code)
    {
    }

    ErrorOr(ErrorCode::code_t code)
        : m_error(code)
    {
    }

    ErrorOr(T value)
        : m_error(ErrorCode::NO_ERROR)
        , m_value(move(value))
    {
    }

    [[nodiscard]] bool is_error() const { return m_error.is_error(); }
    [[nodiscard]] ErrorCode error() const { return m_error; }

    [[nodiscard]] const T& value() const { return m_value; }
    [[nodiscard]] T& value() { return m_value; }

private:
    ErrorCode m_error;
    T m_value {};
};

}
