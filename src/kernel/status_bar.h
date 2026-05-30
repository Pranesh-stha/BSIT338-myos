#ifndef STATUS_BAR_H
#define STATUS_BAR_H

#include "stdint.h"

// Paints the static parts (separator) and the initial status, then
// arms the PIT-hook updater.
void StatusBar_Initialize(void);

// Called from the PIT IRQ handler each tick. Safe before Initialize
// (no-op until armed). 'ticks' = PIT tick count (10 ms each).
void StatusBar_OnTick(uint64_t ticks);

#endif
