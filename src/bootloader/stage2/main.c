#include "stdint.h"
#include "stdio.h"
#include "disk.h"

void __attribute__((cdecl)) start(uint16_t boot_drive)
{
    clrscr();

    printf("Hello from stage2!\n");
    printf("Boot drive: %x\n", boot_drive);

    DISK disk;
    if (!DISK_Initialize(&disk, boot_drive)) {
        printf("DISK_Initialize FAILED\n");
        goto halt;
    }
    printf("Disk geometry: cyl=%u  sec=%u  heads=%u\n",
           disk.cylinders, disk.sectors, disk.heads);

    // Read sector 0 (the boot sector of our floppy) to a low-memory address.
    // 0x20000 = 128 KB — well below 1 MB, and not stepping on stage2 (at 0x500).
    uint8_t* buf = (uint8_t*)0x20000;
    if (!DISK_ReadSectors(&disk, 0, 1, buf)) {
        printf("DISK_ReadSectors FAILED\n");
        goto halt;
    }

    print_buffer("Boot sector first 32 bytes: ", buf, 32);
    printf("\n");
    print_buffer("Last 4 bytes (should end 55 AA): ", buf + 508, 4);

halt:
    for (;;) { __asm__ volatile("hlt"); }
}
