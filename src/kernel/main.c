#include "stdint.h"
#include "stdio.h"
#include "memory.h"
#include "hal/hal.h"
#include "arch/i686/irq.h"
#include "boot/bootparams.h"
#include "pmm.h"

extern uint8_t __bss_start;
extern uint8_t __end;

static void timer_tick(Registers* regs) { (void)regs; }

void __attribute__((section(".entry"))) start(BootParams* bootParams)
{
    memset(&__bss_start, 0, (&__end) - (&__bss_start));

    i686_IRQ_RegisterHandler(0, timer_tick);
    HAL_Initialize();

    clrscr();
    printf("Hello from kernel!\r\n\r\n");

    PMM_Initialize(&bootParams->memory);

    for (;;);
}
