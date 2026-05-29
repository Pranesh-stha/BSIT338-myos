#pragma once

#include "stdint.h"
#include <stdbool.h>

typedef struct {
    uint8_t  id;
    uint16_t cylinders;
    uint16_t sectors;
    uint16_t heads;
} DISK;

bool DISK_Initialize(DISK* disk, uint8_t drive);
bool DISK_ReadSectors(DISK* disk, uint32_t lba, uint8_t sectors, uint8_t* lower_data_out);
