#include "stdlib.h"

int main(int argc, char** argv);

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

    int ret = main(argc, argv);

    exit(ret);
}
