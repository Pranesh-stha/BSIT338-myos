#include "idt.h"
#include "stdint.h"
#include "gdt.h"
#include "util/binary.h"

typedef struct {
    uint16_t BaseLow;          // handler address bits 0-15
    uint16_t SegmentSelector;  // code segment in the GDT
    uint8_t  Reserved;         // always 0
    uint8_t  Flags;            // gate type | ring | present
    uint16_t BaseHigh;         // handler address bits 16-31
} __attribute__((packed)) IDTEntry;

typedef struct {
    uint16_t Limit;            // sizeof(IDT) - 1
    IDTEntry* Ptr;             // address of the table
} __attribute__((packed)) IDTDescriptor;

IDTEntry g_IDT[256];           // all zero by default => every gate "not present"

IDTDescriptor g_IDTDescriptor = { sizeof(g_IDT) - 1, g_IDT };

// implemented in idt.asm
void __attribute__((cdecl)) i686_IDT_Load(IDTDescriptor* idtDescriptor);

void i686_IDT_SetGate(int interrupt, void* base, uint16_t segmentDescriptor, uint8_t flags)
{
    g_IDT[interrupt].BaseLow         = ((uint32_t)base) & 0xFFFF;
    g_IDT[interrupt].SegmentSelector = segmentDescriptor;
    g_IDT[interrupt].Reserved        = 0;
    g_IDT[interrupt].Flags           = flags;
    g_IDT[interrupt].BaseHigh        = (((uint32_t)base) >> 16) & 0xFFFF;
}

void i686_IDT_EnableGate(int interrupt)
{
    FLAG_SET(g_IDT[interrupt].Flags, IDT_FLAG_PRESENT);
}

void i686_IDT_DisableGate(int interrupt)
{
    FLAG_UNSET(g_IDT[interrupt].Flags, IDT_FLAG_PRESENT);
}

void i686_IDT_Initialize()
{
    i686_IDT_Load(&g_IDTDescriptor);
}
