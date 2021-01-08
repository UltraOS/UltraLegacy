ENTRY(start)

SECTIONS
{
    . = 0xC0100000; /* 3 GB + 1MB */

    .text ALIGN(4K) : AT (ADDR (.text) - 0xC0000000)
    {
        *(.entry)
        *(.text .text.*)
    }

    .rodata ALIGN(4K) : AT (ADDR (.rodata) - 0xC0000000)
    {
        global_constructors_begin = .;
        *(.ctors)
        global_constructors_end = .;

        *(.dtors)

        *(.rodata .rodata.*)
    }

    .data ALIGN(4K) : AT (ADDR (.data) - 0xC0000000)
    {
        *(.data .data.*)
    }

    .magic ALIGN(4K) : AT (ADDR (.magic) - 0xC0000000)
    {
        *(.magic)
    }

    .bss ALIGN(4K) : AT (ADDR (.bss) - 0xC0000000)
    {
        section_bss_begin = .;
        *(COMMON)
        *(.bss .bss.*)
        section_bss_end = .;
    }
}
