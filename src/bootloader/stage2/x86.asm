bits 16

section _TEXT class=CODE


;
; U4D - unsigned 4 byte divide (compiler helper)
;
global __U4D
__U4D:
    shl edx, 16
    mov dx, ax
    mov eax, edx
    xor edx, edx
    shl ecx, 16
    mov cx, bx
    div ecx             ; eax = quotient, edx = remainder
    mov ebx, edx        ; CX:BX = remainder
    mov ecx, edx
    shr ecx, 16
    mov edx, eax        ; DX:AX = quotient
    shr edx, 16
    ret


;
; U4M - unsigned 4 byte multiply (compiler helper)
;
global __U4M
__U4M:
    shl edx, 16
    mov dx, ax
    mov eax, edx
    shl ecx, 16
    mov cx, bx
    mul ecx
    mov edx, eax
    shr edx, 16
    mov ebx, eax
    mov ecx, ebx
    shr ecx, 16
    ret


;
; void _cdecl x86_Video_WriteCharTeletype(char c, uint8_t page);
;
global _x86_Video_WriteCharTeletype
_x86_Video_WriteCharTeletype:
    push bp
    mov bp, sp
    push bx

    mov ah, 0Eh
    mov al, [bp + 4]
    mov bh, [bp + 6]
    int 10h

    pop bx
    mov sp, bp
    pop bp
    ret


;
; void _cdecl x86_div64_32(uint64_t dividend, uint32_t divisor, uint64_t* quotientOut, uint32_t* remainderOut);
;
global _x86_div64_32
_x86_div64_32:
    push bp
    mov bp, sp
    push bx

    mov eax, [bp + 8]
    mov ecx, [bp + 12]
    xor edx, edx
    div ecx

    mov bx, [bp + 16]
    mov [bx + 4], eax

    mov eax, [bp + 4]
    div ecx

    mov [bx], eax
    mov bx, [bp + 18]
    mov [bx], edx

    pop bx
    mov sp, bp
    pop bp
    ret


;
; bool _cdecl x86_Disk_Reset(uint8_t drive);
;
global _x86_Disk_Reset
_x86_Disk_Reset:
    push bp
    mov bp, sp

    mov ah, 0
    mov dl, [bp + 4]
    stc
    int 13h

    mov ax, 1
    sbb ax, 0           ; ax = 1 if carry clear (success), 0 if set

    mov sp, bp
    pop bp
    ret


;
; bool _cdecl x86_Disk_Read(uint8_t drive,
;                           uint16_t cylinder,
;                           uint16_t sector,
;                           uint16_t head,
;                           uint8_t count,
;                           void far* dataOut);
;
global _x86_Disk_Read
_x86_Disk_Read:
    push bp
    mov bp, sp

    push bx
    push es

    mov dl, [bp + 4]    ; drive

    mov ch, [bp + 6]    ; cylinder (lower 8 bits)
    mov cl, [bp + 7]    ; cylinder upper 2 bits
    shl cl, 6

    mov al, [bp + 8]    ; sector (lower 6 bits)
    and al, 3Fh
    or cl, al

    mov dh, [bp + 10]   ; head

    mov al, [bp + 12]   ; count

    mov bx, [bp + 16]   ; es segment of far pointer
    mov es, bx
    mov bx, [bp + 14]   ; bx offset of far pointer

    mov ah, 02h
    stc
    int 13h

    mov ax, 1
    sbb ax, 0           ; 1 on success

    pop es
    pop bx

    mov sp, bp
    pop bp
    ret


;
; bool _cdecl x86_Disk_GetDriveParams(uint8_t drive,
;                                     uint8_t* driveTypeOut,
;                                     uint16_t* cylindersOut,
;                                     uint16_t* sectorsOut,
;                                     uint16_t* headsOut);
;
global _x86_Disk_GetDriveParams
_x86_Disk_GetDriveParams:
    push bp
    mov bp, sp

    push es
    push bx
    push si
    push di

    mov dl, [bp + 4]    ; drive
    mov ah, 08h
    mov di, 0           ; es:di = 0:0 to guard against BIOS bugs
    mov es, di
    stc
    int 13h

    ; return value
    mov ax, 1
    sbb ax, 0

    ; out params
    mov si, [bp + 6]    ; drive type from bl
    mov [si], bl

    mov bl, ch          ; cylinders - lower bits in ch
    mov bh, cl          ; upper bits in cl (bits 6-7)
    shr bh, 6
    mov si, [bp + 8]
    mov [si], bx

    xor ch, ch          ; sectors - lower 6 bits in cl
    and cl, 3Fh
    mov si, [bp + 10]
    mov [si], cx

    mov cl, dh          ; heads - in dh
    mov ch, 0
    mov si, [bp + 12]
    mov [si], cx

    pop di
    pop si
    pop bx
    pop es

    mov sp, bp
    pop bp
    ret
