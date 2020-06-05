ENTRY(start)

SECTIONS
{
    . = 0xC0100000; /* 3 GB + 1MB */

    .text ALIGN(4K) : AT (ADDR (.text) - 0xC0000000)
    {
        *(.entry)
        *(.text)
    }

    .rodata ALIGN(4K) : AT (ADDR (.rodata) - 0xC0000000)
    {
        global_constructors_begin = .;
        *(.ctors)
        global_constructors_end = .;

        *(.rodata)
    }

    .data ALIGN(4K) : AT (ADDR (.data) - 0xC0000000)
    {
        *(.data)
    }

    .bss ALIGN(4K) : AT (ADDR (.bss) - 0xC0000000)
    {
        *(COMMON)
        *(.bss)
        *(.kernel_stack)
    }

    .rodata ALIGN(4K) : AT (ADDR (.rodata) - 0xC0000000)
    {
        *(.magic)
    }
}
