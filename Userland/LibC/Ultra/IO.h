#pragma once

#include <stdint.h>

#include <Shared/IO.h>

#define IO_MODE(name, bit) IO_## name = 1 << bit,
enum {
    ENUMERATE_IO_MODES
};
#undef IO_MODE

#define SEEK_MODE(name) SEEK_MODE_## name,
enum {
    ENUMERATE_SEEK_MODES
};
#undef SEEK_MODE

typedef long io_handle;

long io_read(io_handle, void* buffer, unsigned long bytes);
long io_write(io_handle, const void* buffer, unsigned long bytes);
long io_close(io_handle);
long io_seek(io_handle, unsigned long bytes, unsigned long mode);

io_handle open_file(const char* path, long mode);
io_handle create_file(const char* path);
long truncate_file(io_handle, unsigned long bytes);
long remove_file(const char* path);
long move_file(const char* old_path, const char* new_path);
long create_directory(const char* path);
long remove_directory(const char* path);
