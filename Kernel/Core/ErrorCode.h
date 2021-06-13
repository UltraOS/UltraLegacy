#pragma once

#include "Common/String.h"
#include "Common/Types.h"

namespace kernel {

struct ErrorCode {
    enum : u32 {
        NO_ERROR = 0,
        ACCESS_DENIED = 1,
        INVALID_ARGUMENT = 2,
        BAD_PATH = 3,
        DISK_NOT_FOUND = 4,
        UNSUPPORTED = 5,
        NO_SUCH_FILE = 6,
        IS_DIRECTORY = 7,
        IS_FILE = 8,
        FILE_IS_BUSY = 9,
        FILE_ALREADY_EXISTS = 10,
        NAME_TOO_LONG = 11,
        BAD_FILENAME = 12,
    } value { NO_ERROR };

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
    T m_value { };
};

}