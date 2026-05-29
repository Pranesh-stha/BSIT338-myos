#pragma once

#define MEMORY_FAT_ADDR        0x20000
#define MEMORY_FAT_SIZE        0x10000

#define MEMORY_LOAD_KERNEL     ((void*)0x30000)   /* low buffer for BIOS reads */
#define MEMORY_LOAD_SIZE       0x10000

#define MEMORY_KERNEL_ADDR     ((void*)0x100000)  /* where the kernel actually runs */
