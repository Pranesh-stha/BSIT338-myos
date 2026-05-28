#pragma once

// 0x00000000 - 0x000003FF - interrupt vector table
// 0x00000400 - 0x000004FF - BIOS data area

#define MEMORY_MIN              0x00000500
#define MEMORY_MAX              0x00080000

// 0x00000500 - 0x00010500 - FAT driver data (64 KB)
#define MEMORY_FAT_ADDR         ((void far*)0x00500000)
#define MEMORY_FAT_SIZE         0x00010000

// 0x00020000 - 0x00030000 - stage2

// 0x00030000 - ...          - kernel
#define MEMORY_KERNEL_ADDR      ((void far*)0x00300000)
#define MEMORY_LOAD_KERNEL      ((void far*)0x30000)
