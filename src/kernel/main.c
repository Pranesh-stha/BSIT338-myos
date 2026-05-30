#include "stdint.h"
#include "stdio.h"
#include "memory.h"
#include "hal/hal.h"
#include "arch/i686/irq.h"
#include "arch/i686/keyboard.h"
#include "boot/bootparams.h"
#include "pmm.h"

extern uint8_t __bss_start;
extern uint8_t __end;

static void timer_tick(Registers* regs) { (void)regs; }

void __attribute__((section(".entry"))) start(BootParams* bootParams)
{
    memset(&__bss_start, 0, (&__end) - (&__bss_start));

    // Register IRQ handlers before HAL_Initialize STIs.
    i686_IRQ_RegisterHandler(0, timer_tick);
    i686_Keyboard_Initialize();

    HAL_Initialize();

    clrscr();
    PMM_Initialize(&bootParams->memory);

    printf("\r\ndhokaOS ready!\r\n");
    printf("Type something:\r\n\r\n");

    // Echo loop: blocks on the keyboard buffer; IRQ1 fills it asynchronously.
    for (;;)
    {
        char c = i686_Keyboard_GetChar();
        printf("%c", c);
    }
}
