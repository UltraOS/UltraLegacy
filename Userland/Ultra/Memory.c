#include "Syscall.h"

#include <stddef.h>

void* virtual_alloc(void* address, size_t size)
{
    return (void*)syscall_2(SYSCALL_VIRTUAL_ALLOC, (long)address, (long)size);
}

void virtual_free(void* address, size_t size)
{
    syscall_2(SYSCALL_VIRTUAL_FREE, (long)address, (long)size);
}
