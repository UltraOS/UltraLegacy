ENTRY(start)

SECTIONS
{
    /* also defined in UKLoader.asm */
    . = 0x20000;

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.entry)
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
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
}
