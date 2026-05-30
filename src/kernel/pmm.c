#include "pmm.h"
#include "memory.h"
#include "stdio.h"

// Linker-provided symbol: address just past the end of the kernel image
// (after .bss). Defined in linker.ld. We place the bitmap immediately
// after this so kernel memory is never trampled.
extern uint8_t __end;

static uint32_t* g_Bitmap;
static uint32_t  g_BitmapSizeBytes;
static uint32_t  g_TotalBlocks;
static uint32_t  g_UsedBlocks;

// =====================================================
// Bit manipulation helpers
// =====================================================

static inline void PMM_SetBlock(uint32_t block)
{
    g_Bitmap[block / 32] |= (1U << (block % 32));
}

static inline void PMM_ClearBlock(uint32_t block)
{
    g_Bitmap[block / 32] &= ~(1U << (block % 32));
}

static inline bool PMM_TestBlock(uint32_t block)
{
    return (g_Bitmap[block / 32] & (1U << (block % 32))) != 0;
}

// =====================================================
// First-fit: find `count` consecutive free blocks
// =====================================================

static int PMM_FindFreeRegion(uint32_t count)
{
    uint32_t startBlock = 0;
    uint32_t freeCount = 0;

    for (uint32_t i = 0; i < g_TotalBlocks; i++)
    {
        // Optimization: skip whole 32-block chunks that are fully used
        if (i % 32 == 0 && g_Bitmap[i / 32] == 0xFFFFFFFF)
        {
            i += 31;            // loop's ++ will advance to next dword
            freeCount = 0;
            continue;
        }

        if (!PMM_TestBlock(i))
        {
            if (freeCount == 0) startBlock = i;
            freeCount++;
            if (freeCount >= count) return (int)startBlock;
        }
        else
        {
            freeCount = 0;
        }
    }

    return -1;
}

// =====================================================
// Mark a range of blocks as used or free
// =====================================================

static void PMM_MarkBlocksUsed(uint32_t base, uint32_t count)
{
    for (uint32_t i = 0; i < count && (base + i) < g_TotalBlocks; i++)
    {
        if (!PMM_TestBlock(base + i))
        {
            PMM_SetBlock(base + i);
            g_UsedBlocks++;
        }
    }
}

static void PMM_MarkBlocksFree(uint32_t base, uint32_t count)
{
    for (uint32_t i = 0; i < count && (base + i) < g_TotalBlocks; i++)
    {
        if (PMM_TestBlock(base + i))
        {
            PMM_ClearBlock(base + i);
            g_UsedBlocks--;
        }
    }
}

// =====================================================
// Convert byte region -> blocks with safe rounding
// =====================================================

static void PMM_MarkRegionFree(uint64_t base, uint64_t length)
{
    // Round inward: base up, end down
    uint32_t startBlock = (uint32_t)((base + PMM_BLOCK_SIZE - 1) / PMM_BLOCK_SIZE);
    uint32_t endBlock   = (uint32_t)((base + length) / PMM_BLOCK_SIZE);

    if (endBlock <= startBlock) return;
    PMM_MarkBlocksFree(startBlock, endBlock - startBlock);
}

static void PMM_MarkRegionUsed(uint64_t base, uint64_t length)
{
    // Round outward: base down, size up
    uint32_t startBlock = (uint32_t)(base / PMM_BLOCK_SIZE);
    uint32_t blocks     = (uint32_t)((length + PMM_BLOCK_SIZE - 1) / PMM_BLOCK_SIZE);

    PMM_MarkBlocksUsed(startBlock, blocks);
}

// =====================================================
// Initialize
// =====================================================

void PMM_Initialize(MemoryInfo* memoryInfo)
{
    // 1. Find the highest address mentioned in the memory map.
    uint64_t memEnd = 0;
    for (uint32_t i = 0; i < memoryInfo->regionCount; i++)
    {
        uint64_t regionEnd = memoryInfo->regions[i].base + memoryInfo->regions[i].length;
        if (regionEnd > memEnd)
            memEnd = regionEnd;
    }

    // Cap at 4 GB for a 32-bit address space.
    if (memEnd > 0xFFFFFFFFULL)
        memEnd = 0xFFFFFFFFULL;

    g_TotalBlocks     = (uint32_t)(memEnd / PMM_BLOCK_SIZE);
    g_BitmapSizeBytes = (g_TotalBlocks + 7) / 8;
    g_BitmapSizeBytes = (g_BitmapSizeBytes + 3) & ~3U;   // align to 4 bytes

    // 2. Place the bitmap right after the kernel (.bss end), 4-byte aligned.
    g_Bitmap = (uint32_t*)(((uint32_t)&__end + 3) & ~3U);

    // 3. Mark EVERYTHING used initially. Then we'll open up just the
    //    BIOS-reported Usable regions.
    memset(g_Bitmap, 0xFF, g_BitmapSizeBytes);
    g_UsedBlocks = g_TotalBlocks;

    // 4. Walk the memory map and free up each Usable region.
    for (uint32_t i = 0; i < memoryInfo->regionCount; i++)
    {
        if (memoryInfo->regions[i].type == MEMORY_REGION_USABLE)
            PMM_MarkRegionFree(memoryInfo->regions[i].base, memoryInfo->regions[i].length);
    }

    // 5. Re-mark the area from 0 through the end of the bitmap as USED.
    //    That covers IVT, BDA, stage1, stage2, kernel, and the bitmap itself
    //    so nothing later overwrites them.
    uint32_t bitmapEnd = (uint32_t)g_Bitmap + g_BitmapSizeBytes;
    PMM_MarkRegionUsed(0, bitmapEnd);

    printf("PMM: %lu blocks total, %lu used, %lu free (%lu KB free)\r\n",
        g_TotalBlocks,
        g_UsedBlocks,
        g_TotalBlocks - g_UsedBlocks,
        (g_TotalBlocks - g_UsedBlocks) * (PMM_BLOCK_SIZE / 1024));
}

// =====================================================
// Allocate / Free
// =====================================================

void* PMM_AllocateBlock()
{
    int block = PMM_FindFreeRegion(1);
    if (block < 0) return NULL;

    PMM_SetBlock((uint32_t)block);
    g_UsedBlocks++;

    return (void*)((uint32_t)block * PMM_BLOCK_SIZE);
}

void PMM_FreeBlock(void* ptr)
{
    uint32_t block = (uint32_t)ptr / PMM_BLOCK_SIZE;
    if (block >= g_TotalBlocks) return;
    if (!PMM_TestBlock(block)) return;   // already free

    PMM_ClearBlock(block);
    g_UsedBlocks--;
}

bool PMM_IsBlockFree(void* ptr)
{
    uint32_t block = (uint32_t)ptr / PMM_BLOCK_SIZE;
    if (block >= g_TotalBlocks) return false;
    return !PMM_TestBlock(block);
}

void PMM_PrintStats(void)
{
    printf("PMM: %lu blocks total, %lu used, %lu free (%lu KB free)\r\n",
        g_TotalBlocks,
        g_UsedBlocks,
        g_TotalBlocks - g_UsedBlocks,
        (g_TotalBlocks - g_UsedBlocks) * (PMM_BLOCK_SIZE / 1024));
}
