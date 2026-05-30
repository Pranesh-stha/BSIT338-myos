[bits 32]

; =====================================================
; void i686_outb(uint16_t port, uint8_t value)
; =====================================================
global i686_outb
i686_outb:
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret


; =====================================================
; uint8_t i686_inb(uint16_t port)
; =====================================================
global i686_inb
i686_inb:
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret


; =====================================================
; Enable/Disable CPU interrupts
; =====================================================
global i686_EnableInterrupts
i686_EnableInterrupts:
    sti
    ret

global i686_DisableInterrupts
i686_DisableInterrupts:
    cli
    ret
