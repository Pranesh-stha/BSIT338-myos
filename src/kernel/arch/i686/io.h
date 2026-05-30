#pragma once
#include "stdint.h"

void i686_outb(uint16_t port, uint8_t value);
uint8_t i686_inb(uint16_t port);

void i686_EnableInterrupts();
void i686_DisableInterrupts();

// Small delay for slow hardware (write to unused POST diagnostic port 0x80).
// Used between PIC initialization writes — some old chipsets need a moment
// to process each ICW before the next one is accepted.
static inline void i686_io_wait()
{
    i686_outb(0x80, 0);
}
