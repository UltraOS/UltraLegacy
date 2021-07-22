#pragma once

#define ENUMERATE_ERROR_CODES               \
        ERROR_CODE(NO_ERROR, 0)             \
        ERROR_CODE(ACCESS_DENIED, 1)        \
        ERROR_CODE(INVALID_ARGUMENT, 2)     \
        ERROR_CODE(BAD_PATH, 3)             \
        ERROR_CODE(DISK_NOT_FOUND, 4)       \
        ERROR_CODE(UNSUPPORTED, 5)          \
        ERROR_CODE(NO_SUCH_FILE, 6)         \
        ERROR_CODE(IS_DIRECTORY, 7)         \
        ERROR_CODE(IS_FILE, 8)              \
        ERROR_CODE(FILE_IS_BUSY, 9)         \
        ERROR_CODE(FILE_ALREADY_EXISTS, 10) \
        ERROR_CODE(NAME_TOO_LONG, 11)       \
        ERROR_CODE(BAD_FILENAME, 12)        \
        ERROR_CODE(INTERRUPTED, 13)         \
        ERROR_CODE(STREAM_CLOSED, 14)       \
        ERROR_CODE(WOULD_BLOCK_FOREVER, 15)
