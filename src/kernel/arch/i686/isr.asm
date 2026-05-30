[bits 32]

; =====================================================
; Macros for ISR stubs
; =====================================================
%macro ISR_NOERRORCODE 1

[global i686_ISR%1]
i686_ISR%1:
    push 0              ; dummy error code (keep stack consistent)
    push %1             ; interrupt number
    jmp isr_common

%endmacro

%macro ISR_ERRORCODE 1

[global i686_ISR%1]
i686_ISR%1:
                        ; CPU already pushed the error code
    push %1             ; interrupt number
    jmp isr_common

%endmacro

; =====================================================
; Common ISR handler
; =====================================================
[extern i686_ISR_Handler]

isr_common:
    pusha               ; save all general-purpose registers

    xor eax, eax
    mov ax, ds
    push eax            ; save data segment

    mov ax, 0x10        ; load kernel data segment (GDT offset 0x10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; pass pointer to Registers struct as argument
    call i686_ISR_Handler
    ; i686_ISR_Handler now returns a Registers* (eax). Normally same as
    ; what we passed in; when the scheduler decides to context-switch,
    ; eax points at a DIFFERENT task's saved Registers frame. The pop+
    ; popa+iret sequence below then restores that task and resumes it.
    mov esp, eax

    pop eax             ; restore data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                ; restore general-purpose registers
    add esp, 8          ; remove interrupt number + error code
    iret                ; return from interrupt

; =====================================================
; Panic — halt the CPU
; =====================================================
[global i686_Panic]
i686_Panic:
    cli
    hlt
    jmp i686_Panic      ; loop in case of NMI


; =====================================================
; Generated ISR stubs (256 entries)
; Path is relative to where nasm runs (src/kernel), not this file.
; =====================================================
%include "arch/i686/isrs_gen.inc"
