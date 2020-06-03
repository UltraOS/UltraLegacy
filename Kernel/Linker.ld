ENTRY(start)

SECTIONS
{
    /* also defined in UKLoader.asm */
    . = 0x00100000; /* 1 MB */

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.entry)
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        global_constructors_begin = .;
        *(.ctors)
        global_constructors_end = .;

        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.magic)
    }
}
