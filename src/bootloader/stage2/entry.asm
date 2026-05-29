bits 16

section .entry

extern __bss_start
extern __end
extern start
global entry

entry:
    cli                         ; disable interrupts

    ; --- save boot drive (passed in dl by stage1) ---
    mov [g_BootDrive], dl

    ; --- set up real-mode stack ---
    mov ax, ds
    mov ss, ax
    mov sp, 0xFFF0              ; stack just below the 64KB segment boundary
    mov bp, sp

    ; --- switch to protected mode ---
    call EnableA20
    call LoadGDT

    mov eax, cr0
    or al, 1
    mov cr0, eax

    jmp dword 08h:.pmode        ; far jump into 32-bit code segment


.pmode:
    [bits 32]
    ; reload segment registers with 32-bit data selector (offset 0x10)
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; --- set 32-bit stack ---
    ; Must be non-zero (avoid push-wrap) AND below 64KB, so the real-mode
    ; BIOS wrapper functions can read their args via [bp+N] with SS=0
    ; (in 16-bit mode bp is only the low 16 bits of ebp).
    mov esp, 0xFFF0
    mov ebp, esp

    ; --- zero out BSS section ---
    ; required because C code expects uninitialized globals to be zero
    mov edi, __bss_start
    mov ecx, __end
    sub ecx, edi
    xor al, al
    cld
    rep stosb

    ; --- call C start(boot_drive) ---
    xor edx, edx
    mov dl, [g_BootDrive]
    push edx
    call start

    ; if start() returns, halt
.halt:
    cli
    hlt
    jmp .halt


; ============================================================
; A20 gate enable (via keyboard controller)
; ============================================================
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


; ============================================================
; Load GDT
; ============================================================
LoadGDT:
    lgdt [g_GDTDesc]
    ret


; ============================================================
; Constants and data
; ============================================================
KbdControllerDataPort               equ 0x60
KbdControllerCommandPort            equ 0x64
KbdControllerDisableKeyboard        equ 0xAD
KbdControllerEnableKeyboard         equ 0xAE
KbdControllerReadCtrlOutputPort     equ 0xD0
KbdControllerWriteCtrlOutputPort    equ 0xD1


g_GDT:
    dq 0                        ; null descriptor

    ; 32-bit code segment (selector 0x08)
    dw 0FFFFh                   ; limit (bits 0-15)
    dw 0                        ; base (bits 0-15)
    db 0                        ; base (bits 16-23)
    db 10011010b                ; access (present, ring 0, code, executable, readable)
    db 11001111b                ; flags + limit (4K granularity, 32-bit, limit bits 16-19)
    db 0                        ; base (bits 24-31)

    ; 32-bit data segment (selector 0x10)
    dw 0FFFFh
    dw 0
    db 0
    db 10010010b                ; access (present, ring 0, data, writable)
    db 11001111b
    db 0

    ; 16-bit code segment (selector 0x18) — needed later for real-mode switch
    dw 0FFFFh
    dw 0
    db 0
    db 10011010b
    db 00001111b                ; flags: 16-bit, byte granularity
    db 0

    ; 16-bit data segment (selector 0x20)
    dw 0FFFFh
    dw 0
    db 0
    db 10010010b
    db 00001111b
    db 0

g_GDTDesc:
    dw g_GDTDesc - g_GDT - 1
    dd g_GDT


g_BootDrive: db 0
