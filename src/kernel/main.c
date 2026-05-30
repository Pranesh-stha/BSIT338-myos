#include "stdint.h"
#include "stdio.h"
#include "memory.h"
#include "hal/hal.h"
#include "arch/i686/irq.h"
#include "boot/bootparams.h"

extern uint8_t __bss_start;
extern uint8_t __end;

static const char* const g_MemoryRegionTypeStrings[] = {
    "Unknown",
    "Usable",
    "Reserved",
    "ACPI Reclaimable",
    "ACPI NVS",
    "Bad Memory",
};

// No-op timer handler: PIT fires IRQ 0 every ~55 ms regardless. Registering
// a do-nothing handler keeps the dispatcher from spamming "Unhandled IRQ 0".
static void timer_tick(Registers* regs) { (void)regs; }

void __attribute__((section(".entry"))) start(BootParams* bootParams)
{
    // 1. Zero BSS first - g_IDT, g_IRQHandlers, etc. live there.
    memset(&__bss_start, 0, (&__end) - (&__bss_start));

    // 2. Register the timer handler BEFORE HAL_Initialize, since that
    //    call STIs at the end. If we registered after, a PIT tick could
    //    inject "Unhandled IRQ 0" into the memory-map output below.
    i686_IRQ_RegisterHandler(0, timer_tick);

    // 3. GDT + IDT + ISR + IRQ (last step STIs).
    HAL_Initialize();

    clrscr();
    printf("Hello from kernel!\r\n\r\n");

    // Print the memory map that stage2 detected and passed to us.
    printf("Memory regions: %lu\r\n", bootParams->memory.regionCount);
    for (uint32_t i = 0; i < bootParams->memory.regionCount; i++)
    {
        printf("  %llx - %llx  [%s]\r\n",
            bootParams->memory.regions[i].base,
            bootParams->memory.regions[i].base + bootParams->memory.regions[i].length,
            (bootParams->memory.regions[i].type < 6)
                ? g_MemoryRegionTypeStrings[bootParams->memory.regions[i].type]
                : "Unknown");
    }

    for (;;);
}
