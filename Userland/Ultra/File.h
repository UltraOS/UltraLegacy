#pragma once

#include <stdint.h>

#include <Shared/File.h>

#define FILE_MODE(name, bit) FILE_MODE_## name = 1 << bit,
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
long file_create(const char* path);
long file_remove(const char* path);
long file_move(const char* old_path, const char* new_path);
long create_directory(const char* path);
long remove_directory(const char* path);
