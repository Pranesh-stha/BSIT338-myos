#include "stdint.h"
#include "stdio.h"
#include "disk.h"
#include "fat.h"
#include "memdefs.h"

void _cdecl cstart_(uint16_t bootDrive)
{
    DISK disk;
    if (!DISK_Initialize(&disk, bootDrive))
    {
        printf("Disk init error\r\n");
        goto end;
    }

    if (!FAT_Initialize(&disk))
    {
        printf("FAT init error\r\n");
        goto end;
    }

    // browse files in root
    printf("Root directory:\r\n");
    FAT_File far* fd = FAT_Open(&disk, "/");
    FAT_DirectoryEntry entry;
    int i = 0;
    while (FAT_ReadEntry(&disk, fd, &entry) && i++ < 20)
    {
        printf("  ");
        for (int j = 0; j < 11; j++)
            putc(entry.Name[j]);
        printf("\r\n");
    }
    FAT_Close(fd);

    // read test.txt
    char buffer[100];
    uint32_t read;
    fd = FAT_Open(&disk, "test.txt");
    if (fd != NULL)
    {
        printf("test.txt contents:\r\n");
        while ((read = FAT_Read(&disk, fd, sizeof(buffer), buffer)))
        {
            for (uint32_t k = 0; k < read; k++)
            {
                if (buffer[k] == '\n')
                    putc('\r');
                putc(buffer[k]);
            }
        }
        FAT_Close(fd);
    }

    printf("\r\nDone!\r\n");

end:
    for (;;);
}
