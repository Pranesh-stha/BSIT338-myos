#ifndef ARCH_I686_PIT_H
#define ARCH_I686_PIT_H

#include "stdint.h"

#define PIT_FREQUENCY   100     // 100 Hz = 10 ms per tick

void     i686_PIT_Initialize();
uint64_t i686_PIT_GetTicks();
void     i686_PIT_Sleep(uint32_t ms);

#endif
