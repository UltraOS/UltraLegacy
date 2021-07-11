#include "Syscall.h"

void execute_window_command(void* arg0)
{
    syscall_1(SYSCALL_WM_COMMAND, (long)arg0);
}