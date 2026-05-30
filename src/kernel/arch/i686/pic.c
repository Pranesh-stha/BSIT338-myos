#include "pic.h"
#include "io.h"

// =====================================================
// PIC ports
// =====================================================
#define PIC1_COMMAND_PORT    0x20
#define PIC1_DATA_PORT       0x21
#define PIC2_COMMAND_PORT    0xA0
#define PIC2_DATA_PORT       0xA1

// =====================================================
// Initialization Command Words
// =====================================================

// ICW1
enum {
    PIC_ICW1_ICW4           = 0x01,     // ICW4 needed
    PIC_ICW1_SINGLE         = 0x02,     // single mode (0 = cascade)
    PIC_ICW1_INTERVAL4      = 0x04,     // ignored on x86
    PIC_ICW1_LEVEL          = 0x08,     // level triggered (0 = edge)
    PIC_ICW1_INITIALIZE     = 0x10,     // must be set
};

// ICW4
enum {
    PIC_ICW4_8086           = 0x01,     // 8086 mode (vs MCS-80/85)
    PIC_ICW4_AUTO_EOI       = 0x02,     // auto end-of-interrupt
    PIC_ICW4_BUFFER_MASTER  = 0x0C,     // buffered mode, master
    PIC_ICW4_BUFFER_SLAVE   = 0x08,     // buffered mode, slave
    PIC_ICW4_SFNM           = 0x10,     // special fully nested
};

// Operation Command Words
enum {
    PIC_CMD_END_OF_INTERRUPT = 0x20,
    PIC_CMD_READ_IRR         = 0x0A,
    PIC_CMD_READ_ISR         = 0x0B,
};


void i686_PIC_Configure(uint8_t offsetPic1, uint8_t offsetPic2)
{
    // ICW1: start initialization, need ICW4, cascade mode, edge triggered
    i686_outb(PIC1_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    i686_io_wait();
    i686_outb(PIC2_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    i686_io_wait();

    // ICW2: interrupt vector offsets
    i686_outb(PIC1_DATA_PORT, offsetPic1);
    i686_io_wait();
    i686_outb(PIC2_DATA_PORT, offsetPic2);
    i686_io_wait();

    // ICW3: cascading wiring
    i686_outb(PIC1_DATA_PORT, 0x04);    // slave on IRQ2 (bit 2)
    i686_io_wait();
    i686_outb(PIC2_DATA_PORT, 0x02);    // slave identity = 2
    i686_io_wait();

    // ICW4: 8086 mode, manual EOI
    i686_outb(PIC1_DATA_PORT, PIC_ICW4_8086);
    i686_io_wait();
    i686_outb(PIC2_DATA_PORT, PIC_ICW4_8086);
    i686_io_wait();

    // Clear all masks (enable all interrupt lines)
    i686_outb(PIC1_DATA_PORT, 0x00);
    i686_outb(PIC2_DATA_PORT, 0x00);
}

void i686_PIC_SendEndOfInterrupt(int irq)
{
    // If IRQ came from PIC2 (8-15), send EOI to both
    if (irq >= 8)
        i686_outb(PIC2_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);

    // Always send EOI to PIC1
    i686_outb(PIC1_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
}

void i686_PIC_Mask(int irq)
{
    uint16_t port;

    if (irq < 8) {
        port = PIC1_DATA_PORT;
    } else {
        port = PIC2_DATA_PORT;
        irq -= 8;
    }

    uint8_t mask = i686_inb(port);
    i686_outb(port, mask | (1 << irq));
}

void i686_PIC_Unmask(int irq)
{
    uint16_t port;

    if (irq < 8) {
        port = PIC1_DATA_PORT;
    } else {
        port = PIC2_DATA_PORT;
        irq -= 8;
    }

    uint8_t mask = i686_inb(port);
    i686_outb(port, mask & ~(1 << irq));
}

void i686_PIC_Disable()
{
    i686_outb(PIC1_DATA_PORT, 0xFF);
    i686_outb(PIC2_DATA_PORT, 0xFF);
}

uint16_t i686_PIC_ReadIRR()
{
    i686_outb(PIC1_COMMAND_PORT, PIC_CMD_READ_IRR);
    i686_outb(PIC2_COMMAND_PORT, PIC_CMD_READ_IRR);
    return ((uint16_t)i686_inb(PIC2_COMMAND_PORT) << 8)
         | i686_inb(PIC1_COMMAND_PORT);
}

uint16_t i686_PIC_ReadISR()
{
    i686_outb(PIC1_COMMAND_PORT, PIC_CMD_READ_ISR);
    i686_outb(PIC2_COMMAND_PORT, PIC_CMD_READ_ISR);
    return ((uint16_t)i686_inb(PIC2_COMMAND_PORT) << 8)
         | i686_inb(PIC1_COMMAND_PORT);
}
