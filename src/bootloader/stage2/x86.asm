bits 32

;
; void outb(uint16_t port, uint8_t value)
;
global outb
outb:
    mov dx, [esp + 4]               ; port (1st arg, low 16 bits)
    mov al, [esp + 8]               ; value (2nd arg, low 8 bits)
    out dx, al
    ret


;
; uint8_t inb(uint16_t port)
;
global inb
inb:
    mov dx, [esp + 4]               ; port (1st arg)
    xor eax, eax                    ; clear upper bits of return value
    in  al, dx
    ret
