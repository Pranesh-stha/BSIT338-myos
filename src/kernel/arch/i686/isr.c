#include "isr.h"
#include "idt.h"
#include "gdt.h"
#include "stdio.h"

ISRHandler g_ISRHandlers[256];

static const char* const g_Exceptions[] = {
    "Divide by zero",
    "Debug",
    "Non-maskable interrupt",
    "Breakpoint",
    "Overflow",
    "Bound range exceeded",
    "Invalid opcode",
    "Device not available",
    "Double fault",
    "Coprocessor segment overrun",
    "Invalid TSS",
    "Segment not present",
    "Stack-segment fault",
    "General protection fault",
    "Page fault",
    "",
    "x87 floating-point exception",
    "Alignment check",
    "Machine check",
    "SIMD floating-point exception",
    "Virtualization exception",
    "Control protection exception",
    "", "", "", "", "", "",
    "Hypervisor injection exception",
    "VMM communication exception",
    "Security exception",
    "",
};

// forward declaration — implemented in generated C file (Phase 2)
void i686_ISR_InitializeGates();

// called from assembly (isr_common)
void __attribute__((cdecl)) i686_ISR_Handler(Registers* regs)
{
    if (g_ISRHandlers[regs->interrupt] != NULL)
    {
        g_ISRHandlers[regs->interrupt](regs);
    }
    else if (regs->interrupt >= 32)
    {
        printf("Unhandled interrupt %lu!\n", regs->interrupt);
    }
    else
    {
        printf("\n==============================\n");
        printf("  UNHANDLED EXCEPTION %lu: %s\n", regs->interrupt, g_Exceptions[regs->interrupt]);
        printf("==============================\n");
        printf("  eax=%08lx  ebx=%08lx  ecx=%08lx  edx=%08lx\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
        printf("  esi=%08lx  edi=%08lx  ebp=%08lx  esp=%08lx\n", regs->esi, regs->edi, regs->ebp, regs->esp);
        printf("  eip=%08lx  cs=%08lx  efl=%08lx  ss=%08lx\n", regs->eip, regs->cs, regs->eflags, regs->ss);
        printf("  int=%lu  err=%08lx\n", regs->interrupt, regs->error);
        printf("\nKERNEL PANIC!\n");

        i686_Panic();
    }
}

void i686_ISR_Initialize()
{
    i686_ISR_InitializeGates();

    for (int i = 0; i < 256; i++)
    {
        i686_IDT_EnableGate(i);
    }
}

void i686_ISR_RegisterHandler(int interrupt, ISRHandler handler)
{
    g_ISRHandlers[interrupt] = handler;
}
