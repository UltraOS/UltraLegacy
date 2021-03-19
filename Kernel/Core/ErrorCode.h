#pragma once

#include "Common/Types.h"
#include "Common/String.h"

namespace kernel {

struct ErrorCode {
    enum : u32 {
        NO_ERROR = 0,
        ACCESS_DENIED = 1,
        INVALID_ARGUMENT = 2,
        BAD_PATH = 3,
        DISK_NOT_FOUND = 4,
        UNSUPPORTED = 5,
    } value { NO_ERROR };

    ErrorCode() = default;
    ErrorCode(decltype(value) code)
        : value(code)
    {
    }

    bool is_success() const { return value == NO_ERROR; }
    bool is_error() const { return !is_success(); }

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
        default:
            return "<Unknown code>"_sv;
        }
    }
};

}