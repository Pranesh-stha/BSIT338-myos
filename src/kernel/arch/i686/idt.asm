[bits 32]

global i686_IDT_Load
i686_IDT_Load:
    push ebp
    mov ebp, esp

    ; load the IDT (first arg = idt descriptor pointer)
    mov eax, [ebp + 8]
    lidt [eax]

    mov esp, ebp
    pop ebp
    ret
