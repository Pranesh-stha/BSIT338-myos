#ifndef STAGE2_MEM_DETECT_H
#define STAGE2_MEM_DETECT_H

// Renamed from the instruction's "memory.h" to avoid colliding with the
// existing memory.h (memcpy/memset) in this folder.

#include "stdint.h"
#include "boot/bootparams.h"

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) E820MemoryBlock;

#define ASMCALL __attribute__((cdecl))

int ASMCALL x86_E820GetNextBlock(E820MemoryBlock* block, uint32_t* continuationId);

void Memory_Detect(MemoryInfo* memoryInfo);

#endif
