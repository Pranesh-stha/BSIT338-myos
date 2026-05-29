#include "stdint.h"
#include "stdio.h"
#include "memory.h"
#include "memdefs.h"
#include "disk.h"
#include "fat.h"

typedef void (*KernelStart)();

void __attribute__((cdecl)) start(uint16_t boot_drive)
{
    clrscr();

    DISK disk;
    if (!DISK_Initialize(&disk, boot_drive)) { printf("Disk init error\r\n"); goto end; }
    if (!FAT_Initialize(&disk))              { printf("FAT init error\r\n");  goto end; }

    // read kernel.bin into the low buffer, copying each chunk up to 1 MB
    FAT_File* fd = FAT_Open(&disk, "/kernel.bin");
    uint32_t read;
    uint8_t* kernelBuffer = (uint8_t*)MEMORY_KERNEL_ADDR;
    while ((read = FAT_Read(&disk, fd, MEMORY_LOAD_SIZE, MEMORY_LOAD_KERNEL)))
    {
        memcpy(kernelBuffer, MEMORY_LOAD_KERNEL, read);
        kernelBuffer += read;
    }
    FAT_Close(fd);

    // jump into the kernel (cast the constant base address, NOT kernelBuffer,
    // which has been walked past the end of the loaded image)
    KernelStart kernelStart = (KernelStart)MEMORY_KERNEL_ADDR;
    kernelStart();

end:
    for (;;);
}
