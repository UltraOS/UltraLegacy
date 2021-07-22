#pragma once

#include <Shared/Error.h>

#define ERROR_CODE(name, value) ERROR_## name = value,
enum {
    ENUMERATE_ERROR_CODES
};
#undef ERROR_CODE