#include "stdint.h"
#include "stdio.h"
#include "memory.h"
#include "memdefs.h"
#include "disk.h"
#include "fat.h"
#include "mem_detect.h"
#include "boot/bootparams.h"

// Global so the storage persists after start() returns into the kernel.
BootParams g_BootParams;

typedef void (*KernelStart)(BootParams* bootParams);

void __attribute__((cdecl)) start(uint16_t boot_drive)
{
    clrscr();

    DISK disk;
    if (!DISK_Initialize(&disk, boot_drive)) { printf("Disk init error\r\n"); goto end; }
    if (!FAT_Initialize(&disk))              { printf("FAT init error\r\n");  goto end; }

    // Read kernel.bin into the low BIOS buffer, copying each chunk up to 1 MB.
    FAT_File* fd = FAT_Open(&disk, "/kernel.bin");
    uint32_t read;
    uint8_t* kernelBuffer = (uint8_t*)MEMORY_KERNEL_ADDR;
    while ((read = FAT_Read(&disk, fd, MEMORY_LOAD_SIZE, MEMORY_LOAD_KERNEL)))
    {
        memcpy(kernelBuffer, MEMORY_LOAD_KERNEL, read);
        kernelBuffer += read;
    }
    FAT_Close(fd);

    // Walk the E820 memory map and stash it in g_BootParams.memory.
    printf("\r\nDetecting memory...\r\n");
    Memory_Detect(&g_BootParams.memory);

    // Jump to the kernel, passing boot params.
    KernelStart kernelStart = (KernelStart)MEMORY_KERNEL_ADDR;
    kernelStart(&g_BootParams);

end:
    for (;;);
}
