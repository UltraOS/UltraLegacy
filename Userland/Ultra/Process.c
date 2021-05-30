#include "Syscall.h"

long create_process(const char* path)
{
    syscall_1(SYSCALL_CREATE_PROCESS, (long)path);
}

long create_thread(void* entrypoint, void* arg)
{
    syscall_2(SYSCALL_CREATE_THREAD, (long)entrypoint, (long)arg);
}

void process_exit(long code)
{
    syscall_1(SYSCALL_EXIT, (long)code);
}
