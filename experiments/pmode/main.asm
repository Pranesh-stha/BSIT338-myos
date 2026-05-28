org 0x7C00
bits 16


%define ENDL 0x0D, 0x0A


start:
    jmp main


;
; Prints a string to the screen (real mode, BIOS)
;
puts:
    push si
    push ax
    push bx
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp .loop
.done:
    pop bx
    pop ax
    pop si
    ret


main:
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov si, msg_real_mode
    call puts

    ; --- Switch to protected mode ---
    cli                         ; 1. disable interrupts
    call EnableA20              ; 2. enable A20 gate
    call LoadGDT               ; 3. load GDT

    ; 4. set protection enable bit in CR0
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; 5. far jump into 32-bit code segment (offset 8)
    jmp dword 08h:.pmode


.pmode:
    [bits 32]
    ; 6. set up segment registers (data segment = offset 16)
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; print via VGA memory at 0xB8000
    mov esi, msg_pmode
    mov edi, 0xB8000
    mov ah, 0x0F

.print_loop:
    lodsb
    or al, al
    jz .halt
    mov [edi], al
    mov [edi + 1], ah
    add edi, 2
    jmp .print_loop

.halt:
    cli
    hlt


[bits 16]

EnableA20:
    call A20WaitInput
    mov al, KbdControllerDisableKeyboard
    out KbdControllerCommandPort, al

    call A20WaitInput
    mov al, KbdControllerReadCtrlOutputPort
    out KbdControllerCommandPort, al
    call A20WaitOutput
    in al, KbdControllerDataPort
    push eax

    call A20WaitInput
    mov al, KbdControllerWriteCtrlOutputPort
    out KbdControllerCommandPort, al
    call A20WaitInput
    pop eax
    or al, 2
    out KbdControllerDataPort, al

    call A20WaitInput
    mov al, KbdControllerEnableKeyboard
    out KbdControllerCommandPort, al
    call A20WaitInput
    ret

A20WaitInput:
    in al, KbdControllerCommandPort
    test al, 2
    jnz A20WaitInput
    ret

A20WaitOutput:
    in al, KbdControllerCommandPort
    test al, 1
    jz A20WaitOutput
    ret

LoadGDT:
    lgdt [g_GDTDesc]
    ret


KbdControllerDataPort               equ 0x60
KbdControllerCommandPort            equ 0x64
KbdControllerDisableKeyboard        equ 0xAD
KbdControllerEnableKeyboard         equ 0xAE
KbdControllerReadCtrlOutputPort     equ 0xD0
KbdControllerWriteCtrlOutputPort    equ 0xD1


g_GDT:
    dq 0                        ; null descriptor

    ; 32-bit code segment
    dw 0FFFFh
    dw 0
    db 0
    db 10011010b
    db 11001111b
    db 0

    ; 32-bit data segment
    dw 0FFFFh
    dw 0
    db 0
    db 10010010b
    db 11001111b
    db 0

g_GDTDesc:
    dw g_GDTDesc - g_GDT - 1
    dd g_GDT


msg_real_mode:  db 'Hello from REAL mode (BIOS)!', ENDL, 0
msg_pmode:      db 'Hello from 32-bit PROTECTED mode (VGA)!', 0


times 510-($-$$) db 0
dw 0AA55h
