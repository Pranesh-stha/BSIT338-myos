#include "irq.h"
#include "pic.h"
#include "isr.h"
#include "io.h"
#include "stdio.h"

#define PIC_REMAP_OFFSET        0x20    // 32 decimal

IRQHandler g_IRQHandlers[16];

// Called by the ISR dispatcher for interrupts 32-47.
void i686_IRQ_Handler(Registers* regs)
{
    int irq = regs->interrupt - PIC_REMAP_OFFSET;

    if (g_IRQHandlers[irq] != NULL)
    {
        g_IRQHandlers[irq](regs);
    }
    else
    {
        printf("Unhandled IRQ %d\n", irq);
    }

    // MUST send EOI or the PIC will block further interrupts of equal
    // or lower priority. EOI goes to slave first (if applicable), then
    // always to master.
    i686_PIC_SendEndOfInterrupt(irq);
}

void i686_IRQ_Initialize()
{
    // Remap PIC: IRQ 0-7 -> ISR 32-39, IRQ 8-15 -> ISR 40-47.
    i686_PIC_Configure(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8);

    // Route every hardware interrupt vector through our common IRQ handler.
    for (int i = 0; i < 16; i++)
    {
        i686_ISR_RegisterHandler(PIC_REMAP_OFFSET + i, i686_IRQ_Handler);
    }

    // Now safe to enable CPU interrupts.
    i686_EnableInterrupts();
}

void i686_IRQ_RegisterHandler(int irq, IRQHandler handler)
{
    g_IRQHandlers[irq] = handler;
}
