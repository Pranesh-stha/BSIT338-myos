#include "stdint.h"
#include "stdio.h"
#include "memory.h"
#include "hal/hal.h"
#include "arch/i686/irq.h"

extern uint8_t __bss_start;
extern uint8_t __end;

// No-op timer handler: the PIT keeps firing IRQ 0 every ~55 ms regardless
// of what we do. Registering a do-nothing handler keeps the dispatcher
// from falling through to the "Unhandled IRQ 0" path and spamming the
// screen. Replace with a real scheduler tick when we get to Part 12+.
static void timer_tick(Registers* regs) { (void)regs; }

void __attribute__((section(".entry"))) start()
{
    memset(&__bss_start, 0, (&__end) - (&__bss_start));

    HAL_Initialize();

    clrscr();
    printf("Hello from kernel!\r\n");

    i686_IRQ_RegisterHandler(0, timer_tick);

    for (;;);
}
