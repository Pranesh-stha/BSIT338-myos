#include "pit.h"
#include "io.h"
#include "irq.h"
#include "status_bar.h"
#include "scheduler.h"

#define PIT_CHANNEL0_DATA   0x40
#define PIT_COMMAND         0x43
#define PIT_BASE_FREQUENCY  1193182

static volatile uint64_t g_Ticks = 0;

static void PIT_Handler(Registers* regs)
{
    g_Ticks++;
    StatusBar_OnTick(g_Ticks);
    Scheduler_OnTimer(regs);    // may set g_SchedulerNextTask to switch
}

void i686_PIT_Initialize()
{
    uint16_t divisor = (uint16_t)(PIT_BASE_FREQUENCY / PIT_FREQUENCY);

    // Command byte:
    //   bit 6-7 = 00  : channel 0
    //   bit 4-5 = 11  : lo-byte then hi-byte access
    //   bit 1-3 = 010 : mode 2 (rate generator)
    //   bit 0   = 0   : binary count
    // => 0x36
    i686_outb(PIT_COMMAND, 0x36);

    // Send divisor in two bytes (little-endian)
    i686_outb(PIT_CHANNEL0_DATA, (uint8_t)(divisor & 0xFF));
    i686_outb(PIT_CHANNEL0_DATA, (uint8_t)((divisor >> 8) & 0xFF));

    // Take over IRQ 0
    i686_IRQ_RegisterHandler(0, PIT_Handler);
}

uint64_t i686_PIT_GetTicks()
{
    return g_Ticks;
}

void i686_PIT_Sleep(uint32_t ms)
{
    // Busy-wait. Tick increments happen inside the IRQ handler with
    // interrupts disabled, so reads of g_Ticks are never torn between
    // its high and low halves on this single-CPU setup.
    uint64_t target = g_Ticks + ((uint64_t)ms * PIT_FREQUENCY) / 1000;
    while (g_Ticks < target)
        ;
}
