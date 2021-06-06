#include "stdlib.h"

int main(int argc, char** argv);

void _init();
void stdio_init();
void malloc_init();

void _start()
{
    malloc_init();
    stdio_init();
    _init();

    char arg1[] = { "/fixme.exe" };
    char* argv[] = { arg1 };

    int ret = main(1, argv);

    exit(ret);
}
