ENTRY(start)

SECTIONS
{
    . = 0x100000;            /* 1 MB */

    .entry   : { *(.entry) }
    .text    : { *(.text) *(.text.*) }
    .data    : { *(.data) }
    .rodata  : { *(.rodata) *(.rodata.*) }

    __bss_start = .;
    .bss     : { *(COMMON) *(.bss) }
    __end = .;
}
