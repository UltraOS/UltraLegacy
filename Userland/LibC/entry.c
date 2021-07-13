#include "stdlib.h"

int main(int argc, char** argv, char** envp);

void _init();
void stdio_init();
void malloc_init();

void libc_entrypoint(void* sysv_stack)
{
    malloc_init();
    stdio_init();
    _init();

    int argc = *(int*)sysv_stack;
    char** argv = (char**)((char*)sysv_stack + sizeof(size_t));
    char** envp = (char**)((char*)sysv_stack + (sizeof(size_t) * (argc + 2))); // 2 to skip over argc and argv[argc]

    int ret = main(argc, argv, envp);

    exit(ret);
}
