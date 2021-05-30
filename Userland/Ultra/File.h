#pragma once

#include <stdint.h>

#include <Shared/File.h>

#define FILE_MODE(name) FILE_MODE_## name,
enum {
    ENUMERATE_FILE_MODES
};
#undef FILE_MODE

#define SEEK_MODE(name) SEEK_MODE_## name,
enum {
    ENUMERATE_SEEK_MODES
};
#undef SEEK_MODE

long file_open(const char* path, long mode);
long file_read(uint32_t fd, void* buffer, uint32_t bytes);
long file_write(uint32_t fd, void* buffer, uint32_t bytes);
long file_close(uint32_t fd);
long file_seek(uint32_t fd, uint32_t bytes, uint32_t mode);
long file_trucate(uint32_t fd, uint32_t bytes);
