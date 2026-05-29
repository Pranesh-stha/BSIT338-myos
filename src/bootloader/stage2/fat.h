#pragma once
#include "stdint.h"
#include "disk.h"

#define FAT_ATTRIBUTE_READ_ONLY     0x01
#define FAT_ATTRIBUTE_HIDDEN        0x02
#define FAT_ATTRIBUTE_SYSTEM        0x04
#define FAT_ATTRIBUTE_VOLUME_ID     0x08
#define FAT_ATTRIBUTE_DIRECTORY     0x10
#define FAT_ATTRIBUTE_ARCHIVE       0x20
#define FAT_ATTRIBUTE_LFN           (FAT_ATTRIBUTE_READ_ONLY | \
                                     FAT_ATTRIBUTE_HIDDEN | \
                                     FAT_ATTRIBUTE_SYSTEM | \
                                     FAT_ATTRIBUTE_VOLUME_ID)

typedef struct
{
    uint8_t Name[11];
    uint8_t Attributes;
    uint8_t _Reserved;
    uint8_t CreatedTimeTenths;
    uint16_t CreatedTime;
    uint16_t CreatedDate;
    uint16_t AccessedDate;
    uint16_t FirstClusterHigh;
    uint16_t ModifiedTime;
    uint16_t ModifiedDate;
    uint16_t FirstClusterLow;
    uint32_t Size;
} __attribute__((packed)) FAT_DirectoryEntry;

typedef struct
{
    int Handle;
    bool IsDirectory;
    uint32_t Position;
    uint32_t Size;
} FAT_File;

bool FAT_Initialize(DISK* disk);
FAT_File* FAT_Open(DISK* disk, const char* path);
uint32_t FAT_Read(DISK* disk, FAT_File* file, uint32_t byteCount, void* dataOut);
bool FAT_ReadEntry(DISK* disk, FAT_File* file, FAT_DirectoryEntry* dirEntry);
void FAT_Close(FAT_File* file);
