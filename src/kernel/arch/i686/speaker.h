#ifndef ARCH_I686_SPEAKER_H
#define ARCH_I686_SPEAKER_H

#include "stdint.h"

// PC speaker via PIT channel 2 + gate bits in port 0x61.
// Sound is only audible if QEMU was launched with PC-speaker audio.

void Speaker_PlayFreq(uint32_t hz);   // start a square wave at this frequency
void Speaker_Off(void);               // silence

// Convenience: play a tone for `ms` milliseconds, then silence.
void Speaker_Beep(uint32_t hz, uint32_t ms);

// Plays a short canned melody. Returns when done.
void Speaker_PlayMelody(void);

#endif
