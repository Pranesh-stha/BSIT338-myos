#ifndef BOOT_BOOTPARAMS_H
#define BOOT_BOOTPARAMS_H

// Use the local stdint.h provided by stage2/kernel (NOT system <stdint.h>),
// otherwise we get duplicate-typedef errors under -std=c99 because both
// projects already include their own stdint.h with the same typedefs.
// The quoted include falls through -I. -> finds the per-project copy.
#include "stdint.h"

typedef enum {
    MEMORY_REGION_USABLE           = 1,
    MEMORY_REGION_RESERVED         = 2,
    MEMORY_REGION_ACPI_RECLAIMABLE = 3,
    MEMORY_REGION_ACPI_NVS         = 4,
    MEMORY_REGION_BAD              = 5,
} MemoryRegionType;

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) MemoryRegion;

typedef struct {
    uint32_t      regionCount;
    MemoryRegion* regions;
} MemoryInfo;

typedef struct {
    MemoryInfo memory;
} BootParams;

#endif
