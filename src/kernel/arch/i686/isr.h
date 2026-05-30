#ifndef ARCH_I686_ISR_H
#define ARCH_I686_ISR_H

#include "stdint.h"

typedef struct
{
    // in reverse order of how they were pushed:
    uint32_t ds;                                            // pushed by us
    uint32_t edi, esi, ebp, kern_esp, ebx, edx, ecx, eax;  // pusha
    uint32_t interrupt, error;                              // pushed by stub
    uint32_t eip, cs, eflags, esp, ss;                      // pushed by CPU

} __attribute__((packed)) Registers;

typedef void (*ISRHandler)(Registers* regs);

void i686_ISR_Initialize();
void i686_ISR_RegisterHandler(int interrupt, ISRHandler handler);

void i686_Panic();   // implemented in isr.asm

#endif
