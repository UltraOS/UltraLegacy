ENTRY(start)

SECTIONS
{
    /* also defined in UKLoader.asm */
    . = 0x20000;

    .entry BLOCK(4K) : ALIGN(4K)
    {
        *(.entry)
    }

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        global_constructors_begin = .;
        KEEP(*(.init_array));
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)));
        global_constructors_end = .;

        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    /DISCARD/ : { *(.fini_array*) *(.comment) }
}
