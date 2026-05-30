#include "mem_detect.h"
#include "stdio.h"

#define MAX_MEMORY_REGIONS  256

static MemoryRegion g_MemRegions[MAX_MEMORY_REGIONS];

static const char* const g_MemoryRegionTypeStrings[] = {
    "Unknown",
    "Usable",
    "Reserved",
    "ACPI Reclaimable",
    "ACPI NVS",
    "Bad Memory",
};

void Memory_Detect(MemoryInfo* memoryInfo)
{
    E820MemoryBlock block;
    uint32_t continuation = 0;
    int regionCount = 0;
    int ret;

    while ((ret = x86_E820GetNextBlock(&block, &continuation)) > 0)
    {
        g_MemRegions[regionCount].base   = block.base;
        g_MemRegions[regionCount].length = block.length;
        g_MemRegions[regionCount].type   = block.type;
        // BIOS reports ACPI flags only when it filled the 24-byte form;
        // 20-byte responses don't include them — assume "valid" (1).
        g_MemRegions[regionCount].acpi   = (ret > 20) ? block.acpi : 1;
        regionCount++;

        if (regionCount >= MAX_MEMORY_REGIONS || continuation == 0)
            break;
    }

    printf("Memory map (%d regions):\r\n", regionCount);
    for (int i = 0; i < regionCount; i++)
    {
        printf("  %llx - %llx  [%s]\r\n",
            g_MemRegions[i].base,
            g_MemRegions[i].base + g_MemRegions[i].length,
            (g_MemRegions[i].type < 6) ? g_MemoryRegionTypeStrings[g_MemRegions[i].type] : "Unknown");
    }

    memoryInfo->regionCount = regionCount;
    memoryInfo->regions     = g_MemRegions;
}
