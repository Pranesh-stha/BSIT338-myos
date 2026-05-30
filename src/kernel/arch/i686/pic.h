#ifndef ARCH_I686_PIC_H
#define ARCH_I686_PIC_H

#include "stdint.h"

void i686_PIC_Configure(uint8_t offsetPic1, uint8_t offsetPic2);
void i686_PIC_SendEndOfInterrupt(int irq);
void i686_PIC_Mask(int irq);
void i686_PIC_Unmask(int irq);
void i686_PIC_Disable();
uint16_t i686_PIC_ReadIRR();
uint16_t i686_PIC_ReadISR();

#endif
