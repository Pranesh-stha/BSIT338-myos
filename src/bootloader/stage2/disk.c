#include "disk.h"
#include "x86.h"
#include "stdio.h"

bool DISK_Initialize(DISK* disk, uint8_t drive)
{
    uint8_t  drive_type;
    uint16_t cylinders, sectors, heads;

    if (!x86_Disk_GetDriveParams(drive, &drive_type, &cylinders, &sectors, &heads)) {
        return false;
    }

    disk->id        = drive;
    disk->cylinders = cylinders;
    disk->sectors   = sectors;
    disk->heads     = heads;
    return true;
}

// Convert LBA → CHS using the disk geometry.
// LBA = (C * heads + H) * sectors_per_track + (S - 1)
static void DISK_LBA2CHS(DISK* disk, uint32_t lba,
                         uint16_t* cylinder_out,
                         uint16_t* sector_out,
                         uint16_t* head_out)
{
    *sector_out   = lba % disk->sectors + 1;
    uint32_t tmp  = lba / disk->sectors;
    *head_out     = tmp % disk->heads;
    *cylinder_out = tmp / disk->heads;
}

bool DISK_ReadSectors(DISK* disk, uint32_t lba, uint8_t sectors, uint8_t* lower_data_out)
{
    uint16_t cylinder, sector, head;
    DISK_LBA2CHS(disk, lba, &cylinder, &sector, &head);

    // Retry up to 3 times — floppies are unreliable, so always retry
    for (int i = 0; i < 3; i++) {
        if (x86_Disk_Read(disk->id, cylinder, sector, head, sectors, lower_data_out))
            return true;
        x86_Disk_Reset(disk->id);
    }

    return false;
}
