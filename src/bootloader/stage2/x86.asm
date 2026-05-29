bits 32


; ============================================================
; Constants for segment selectors in our GDT
; ============================================================
%define CODE32_SEG  0x08
%define DATA32_SEG  0x10
%define CODE16_SEG  0x18
%define DATA16_SEG  0x20


; ============================================================
; Macro: x86_EnterRealMode
;   Used inside a function. Switches CPU from 32-bit protected
;   mode → 16-bit protected mode → 16-bit real mode.
;   After this macro, you can call BIOS interrupts.
; ============================================================
%macro x86_EnterRealMode 0
    [bits 32]
    jmp word CODE16_SEG:.pmode16        ; jump to 16-bit protected mode

.pmode16:
    [bits 16]
    ; disable protected mode bit in CR0
    mov eax, cr0
    and al, ~1
    mov cr0, eax

    ; far jump to real mode (CS = 0)
    jmp word 0x00:.rmode

.rmode:
    ; reset segment registers to real-mode values
    xor ax, ax
    mov ds, ax
    mov ss, ax

    sti                                  ; enable interrupts (BIOS needs them)
%endmacro


; ============================================================
; Macro: x86_EnterProtectedMode
;   Reverse of above. Used inside a function that just
;   finished a BIOS call. Returns CPU to 32-bit protected mode.
; ============================================================
%macro x86_EnterProtectedMode 0
    cli                                  ; disable interrupts

    ; re-enable protected mode bit in CR0
    mov eax, cr0
    or al, 1
    mov cr0, eax

    ; far jump into 32-bit code segment
    jmp dword CODE32_SEG:.pmode

.pmode:
    [bits 32]
    ; restore 32-bit segment registers
    mov ax, DATA32_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
%endmacro


; ============================================================
; Macro: LinearToSegOffset
;   Convert a linear (flat) address into a real-mode segment:offset pair.
;   The trick: segment = addr >> 4, offset = addr & 0xF.
;
;   Args:
;     %1 - linear address (a stack offset like [bp + 12])
;     %2 - output segment register (e.g. es)
;     %3 - register that aliases the segment as a normal reg (e.g. eax)
;     %4 - output offset register (e.g. esi)
;
;   NOTE: linear address must be below 1 MB so it fits in real-mode addressing.
; ============================================================
%macro LinearToSegOffset 4
    mov %3, %1            ; linear address -> intermediary reg
    shr %3, 4             ; divide by 16 to get segment
    mov %2, %3            ; segment -> target segment reg
    mov %3, %1            ; linear address -> intermediary reg again
    and %3, 0xF           ; keep low 4 bits as offset
    mov %4, %3            ; offset -> target offset reg
%endmacro


; ============================================================
; void outb(uint16_t port, uint8_t value)
; ============================================================
global outb
outb:
    [bits 32]
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret


; ============================================================
; uint8_t inb(uint16_t port)
; ============================================================
global inb
inb:
    [bits 32]
    mov dx, [esp + 4]
    xor eax, eax
    in  al, dx
    ret


; ============================================================
; bool _cdecl x86_Disk_GetDriveParams(uint8_t drive,
;                                     uint8_t* drive_type_out,
;                                     uint16_t* cylinders_out,
;                                     uint16_t* sectors_out,
;                                     uint16_t* heads_out)
;
; Calls BIOS int 0x13 / AH=0x08 to query disk geometry.
; Returns true on success.
; ============================================================
global x86_Disk_GetDriveParams
x86_Disk_GetDriveParams:
    [bits 32]
    push ebp
    mov  ebp, esp

    ; save registers we'll clobber
    push ebx
    push esi
    push edi
    push es

    x86_EnterRealMode

    ; --- BIOS call setup ---
    [bits 16]
    mov dl, [bp + 8]              ; arg 1: drive
    mov ah, 0x08                  ; function: get drive parameters
    mov di, 0                     ; ES:DI must be 0:0 per BIOS quirks
    mov es, di
    stc                           ; set carry so we can detect failure
    int 0x13

    ; --- store results before they get clobbered ---
    ; if CF=1, BIOS reported an error
    mov eax, 1
    sbb eax, 0                    ; eax = 1 - CF (so eax=0 on error, 1 on success)

    ; drive_type returned in BL → store at [bp+12]
    LinearToSegOffset [bp + 12], es, edi, edi
    mov [es:di], bl

    ; cylinders: high 2 bits of CL + all of CH, +1
    mov bl, ch
    mov bh, cl
    shr bh, 6
    inc bx
    LinearToSegOffset [bp + 16], es, edi, edi
    mov [es:di], bx

    ; sectors: low 6 bits of CL
    xor ch, ch
    and cl, 0x3F
    LinearToSegOffset [bp + 20], es, edi, edi
    mov [es:di], cx

    ; heads: DH + 1
    mov cl, dh
    xor ch, ch
    inc cx
    LinearToSegOffset [bp + 24], es, edi, edi
    mov [es:di], cx

    push eax                      ; save return value across mode switch

    x86_EnterProtectedMode

    [bits 32]
    pop eax

    pop es
    pop edi
    pop esi
    pop ebx

    mov esp, ebp
    pop ebp
    ret


; ============================================================
; bool _cdecl x86_Disk_Reset(uint8_t drive)
; ============================================================
global x86_Disk_Reset
x86_Disk_Reset:
    [bits 32]
    push ebp
    mov  ebp, esp

    x86_EnterRealMode

    [bits 16]
    mov ah, 0
    mov dl, [bp + 8]
    stc
    int 0x13

    mov eax, 1
    sbb eax, 0

    push eax
    x86_EnterProtectedMode

    [bits 32]
    pop eax

    mov esp, ebp
    pop ebp
    ret


; ============================================================
; bool _cdecl x86_Disk_Read(uint8_t drive,
;                           uint16_t cylinder,
;                           uint16_t sector,
;                           uint16_t head,
;                           uint8_t count,
;                           uint8_t* data_out)
;
; Reads `count` sectors using CHS addressing into data_out.
; data_out MUST point to memory below 1 MB.
; ============================================================
global x86_Disk_Read
x86_Disk_Read:
    [bits 32]
    push ebp
    mov  ebp, esp

    push ebx
    push esi
    push edi
    push es

    x86_EnterRealMode
    [bits 16]

    mov dl, [bp + 8]              ; drive

    ; cylinder (16-bit): low 8 in CH, high 2 in top of CL
    mov ax, [bp + 12]
    mov ch, al
    shl ah, 6
    or  ah, ah                    ; place 2 high bits into CL later

    ; sector (1-6) in low 6 bits of CL
    mov al, [bp + 16]             ; sector
    and al, 0x3F
    or  al, ah                    ; combine sector with cylinder-high
    mov cl, al

    ; head in DH
    mov dh, [bp + 20]             ; head (low byte)

    ; count (sectors to read) in AL
    mov al, [bp + 24]

    ; data pointer → ES:BX
    LinearToSegOffset [bp + 28], es, ebx, ebx

    mov ah, 0x02                  ; function: read sectors
    stc
    int 0x13

    mov eax, 1
    sbb eax, 0

    push eax
    x86_EnterProtectedMode

    [bits 32]
    pop eax

    pop es
    pop edi
    pop esi
    pop ebx

    mov esp, ebp
    pop ebp
    ret
