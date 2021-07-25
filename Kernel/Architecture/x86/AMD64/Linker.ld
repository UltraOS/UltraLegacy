ENTRY(start)

SECTIONS
{
    kernel_space_begin = 0xFFFFFFFF80000000; /* MAX - 2GB */
    . = kernel_space_begin + 0x100000; /* 1MB into the address space */

    .text ALIGN(4K) : AT (ADDR (.text) - kernel_space_begin)
    {
        *(.entry)
        *(.text .text.*)
        safe_operations_begin = .;
        *(.safe_operations)
        safe_operations_end = .;
    }

    .rodata ALIGN(4K) : AT (ADDR (.rodata) - kernel_space_begin)
    {
        global_constructors_begin = .;
        *(.ctors)
        global_constructors_end = .;

        *(.dtors)

        *(.rodata .rodata.*)
    }

    .data ALIGN(4K) : AT (ADDR (.data) - kernel_space_begin)
    {
        *(.data .data.*)
    }

    .magic ALIGN(4K) : AT (ADDR (.magic) - kernel_space_begin)
    {
        *(.magic)
    }

    .bss ALIGN(4K) : AT (ADDR (.bss) - kernel_space_begin)
    {
        section_bss_begin = .;
        *(COMMON)
        *(.bss .bss.*)
        section_bss_end = .;
    }
}
