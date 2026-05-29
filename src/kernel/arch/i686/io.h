#pragma once
#include "stdint.h"

void i686_outb(uint16_t port, uint8_t value);
uint8_t i686_inb(uint16_t port);
