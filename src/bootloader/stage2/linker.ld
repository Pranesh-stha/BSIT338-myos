ENTRY(entry)

phys = 0x500;

SECTIONS
{
    . = phys;

    .entry : {
        __entry_start = .;
        *(.entry)
    }

    .text : {
        __text_start = .;
        *(.text)
        *(.text.*)
    }

    .data : {
        __data_start = .;
        *(.data)
    }

    .rodata : {
        __rodata_start = .;
        *(.rodata)
        *(.rodata.*)
    }

    .bss : {
        __bss_start = .;
        *(COMMON)
        *(.bss)
        __bss_end = .;
    }

    __end = .;
}
