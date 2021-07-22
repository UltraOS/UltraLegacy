#include "Syscall.h"
#include "IO.h"

#include <stdint.h>

long open_file(const char* path, long mode)
{
    return syscall_2(SYSCALL_OPEN, (long)path, mode);
}

long io_read(io_handle handle, void* buffer, unsigned long bytes)
{
    return syscall_3(SYSCALL_READ, handle, (long)buffer, (long)bytes);
}

long io_write(io_handle handle, const void* buffer, unsigned long bytes)
{
    return syscall_3(SYSCALL_WRITE, handle, (long)buffer, (long)bytes);
}

long io_close(io_handle handle)
{
    return syscall_1(SYSCALL_CLOSE, handle);
}

long io_seek(io_handle handle, unsigned long bytes, unsigned long mode)
{
    return syscall_3(SYSCALL_SEEK, handle, (long)bytes, (long)mode);
}

long io_truncate(io_handle handle, unsigned long bytes)
{
    return syscall_2(SYSCALL_TRUNCATE, handle, bytes);
}

long create_file(const char* path)
{
    return syscall_1(SYSCALL_CREATE, (long)path);
}

long remove_file(const char* path)
{
    return syscall_1(SYSCALL_REMOVE, (long)path);
}

long move_file(const char* old_path, const char* new_path)
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
