#ifndef ARCH_I686_IRQ_H
#define ARCH_I686_IRQ_H

#include "isr.h"

typedef void (*IRQHandler)(Registers* regs);

void i686_IRQ_Initialize();
void i686_IRQ_RegisterHandler(int irq, IRQHandler handler);

#endif
