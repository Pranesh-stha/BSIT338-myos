#include "stdint.h"
#include "stdio.h"

void __attribute__((cdecl)) start(uint16_t boot_drive)
{
    clrscr();

    printf("Hello from stage2!\n");
    printf("Boot drive: %x\n", boot_drive);
    printf("Numbers: %d  %u  %x  %o\n", -1234, 5678, 0xDEAD, 0777);
    printf("Strings: %s and %s\n", "hello", "world");
    printf("64-bit math: %llu\n", 1234567890123ULL);
    printf("Char: %c, Percent: %%\n", 'A');
    printf("\nScroll test:\n");

    for (int i = 0; i < 30; i++) {
        printf("  Line %d\n", i);
    }

    for (;;) { __asm__ volatile("hlt"); }
}
