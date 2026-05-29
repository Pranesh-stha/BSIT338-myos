bits 32


; ============================================================
; void outb(uint16_t port, uint8_t value)
; ============================================================
global outb
outb:
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret


; ============================================================
; uint8_t inb(uint16_t port)
; ============================================================
global inb
inb:
    mov dx, [esp + 4]
    xor eax, eax
    in  al, dx
    ret
