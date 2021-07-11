#include "Syscall.h"

#include <stdint.h>

long file_open(const char* path, long mode)
{
    return syscall_2(SYSCALL_OPEN, (long)path, mode);
}

long file_read(uint32_t fd, void* buffer, uint32_t bytes)
{
    return syscall_3(SYSCALL_READ, (long)fd, (long)buffer, (long)bytes);
}

long file_write(uint32_t fd, void* buffer, uint32_t bytes)
{
    return syscall_3(SYSCALL_WRITE, (long)fd, (long)buffer, (long)bytes);
}

long file_close(uint32_t fd)
{
    return syscall_1(SYSCALL_CLOSE, (long)fd);
}

long file_seek(uint32_t fd, uint32_t bytes, uint32_t mode)
{
    return syscall_3(SYSCALL_SEEK, (long)fd, (long)bytes, (long)mode);
}

long file_create(const char* path)
{
    return syscall_1(SYSCALL_CREATE, (long)path);
}

long file_remove(const char* path)
{
    return syscall_1(SYSCALL_REMOVE, (long)path);
}

long file_move(const char* old_path, const char* new_path)
{
    return syscall_2(SYSCALL_MOVE, (long)old_path, (long)new_path);
}

long create_directory(const char* path)
{
    return syscall_1(SYSCALL_CREATE_DIR, (long)path);
}

long remove_directory(const char* path)
{
    return syscall_1(SYSCALL_REMOVE_DIR, (long)path);
}

void debug_log(const char* message)
{
    syscall_1(SYSCALL_DEBUG_LOG, (long)message);
}
