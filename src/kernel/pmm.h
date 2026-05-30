#ifndef PMM_H
#define PMM_H

#include "stdint.h"
#include <stdbool.h>
#include "boot/bootparams.h"

#define PMM_BLOCK_SIZE  4096

void  PMM_Initialize(MemoryInfo* memoryInfo);
void* PMM_AllocateBlock();
void  PMM_FreeBlock(void* ptr);
bool  PMM_IsBlockFree(void* ptr);

#endif
