[bits 32]

global i686_outb
i686_outb:
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

global i686_inb
i686_inb:
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret
