#include "stdint.h"

void __attribute__((cdecl)) start(uint16_t boot_drive)
{
    // Direct VGA write: spell out "Hello from stage2!" at top-left of screen.
    // This proves: GCC compiled, linker linked, entry.asm called us,
    // and we are in 32-bit protected mode.
    volatile uint8_t *vga = (volatile uint8_t *)0xB8000;
    const char *msg = "Hello from stage2!";
    int i = 0;
    while (msg[i]) {
        vga[i * 2]     = msg[i];
        vga[i * 2 + 1] = 0x0F;      // white on black
        i++;
    }

    // Halt — we'll add proper exit logic later.
    for (;;) { __asm__ volatile("hlt"); }

    (void)boot_drive;               // suppress unused-param warning
}
