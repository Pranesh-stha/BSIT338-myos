org 0x7C00          ; Tell assembler our code will be loaded at 0x7C00
bits 16             ; Emit 16-bit code (real mode)

%define ENDL 0x0D, 0x0A   ; Carriage return + line feed macro

main:
    ; Setup data segments
    mov ax, 0       ; Can't write 0 directly to ds/es, use ax as intermediary
    mov ds, ax
    mov es, ax

    ; Setup stack
    mov ss, ax
    mov sp, 0x7C00  ; Stack grows downward from our code's start

    ; Print hello message
    mov si, msg_hello
    call puts

.halt:
    hlt             ; Halt the CPU
    jmp .halt       ; If we wake up, loop forever


; puts: prints a null-terminated string
; Input: ds:si points to the string
puts:
    push si
    push ax
    push bx

.loop:
    lodsb           ; Load byte from ds:si into al, increment si
    or al, al       ; Check if al is zero (end of string)
    jz .done

    mov ah, 0x0E    ; BIOS teletype function
    mov bh, 0       ; Page number 0
    int 0x10        ; Call BIOS video interrupt

    jmp .loop

.done:
    pop bx
    pop ax
    pop si
    ret


msg_hello: db 'Hello world from MyOS!', ENDL, 0

times 510-($-$$) db 0   ; Pad the rest of the sector with zeros
dw 0AA55h               ; Boot sector signature (BIOS requires this)
