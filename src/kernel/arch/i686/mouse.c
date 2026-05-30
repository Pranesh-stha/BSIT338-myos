#include "mouse.h"
#include "io.h"
#include "irq.h"
#include "stdio.h"

#define PS2_DATA_PORT       0x60
#define PS2_STATUS_PORT     0x64
#define PS2_COMMAND_PORT    0x64

// =====================================================
// PS/2 controller helpers
// =====================================================

static void PS2_WaitWrite(void)
{
    for (int i = 0; i < 100000; i++)
        if (!(i686_inb(PS2_STATUS_PORT) & 0x02))
            return;
}

static void PS2_WaitRead(void)
{
    for (int i = 0; i < 100000; i++)
        if (i686_inb(PS2_STATUS_PORT) & 0x01)
            return;
}

static void Mouse_SendCommand(uint8_t cmd)
{
    PS2_WaitWrite();
    i686_outb(PS2_COMMAND_PORT, 0xD4);  // "next data-port byte goes to mouse"
    PS2_WaitWrite();
    i686_outb(PS2_DATA_PORT, cmd);
}

static uint8_t Mouse_ReadResponse(void)
{
    PS2_WaitRead();
    return i686_inb(PS2_DATA_PORT);
}

// =====================================================
// Mouse state (initial pos: center of 80x25 text mode)
// =====================================================

static volatile int32_t g_MouseX = 40;
static volatile int32_t g_MouseY = 12;
static volatile bool    g_LeftBtn   = false;
static volatile bool    g_RightBtn  = false;
static volatile bool    g_MiddleBtn = false;

// 3-byte packet accumulator
static uint8_t g_Packet[3];
static int     g_PacketCycle = 0;

// =====================================================
// IRQ12 handler - fires once per BYTE, not per packet
// =====================================================

static void Mouse_Handler(Registers* regs)
{
    (void)regs;
    uint8_t data = i686_inb(PS2_DATA_PORT);

    switch (g_PacketCycle)
    {
        case 0:
            // Byte 0: bit 3 is always 1 in a valid first byte; use as resync.
            if (data & 0x08)
            {
                g_Packet[0] = data;
                g_PacketCycle = 1;
            }
            break;

        case 1:
            g_Packet[1] = data;     // X delta
            g_PacketCycle = 2;
            break;

        case 2:
        {
            g_Packet[2] = data;     // Y delta
            g_PacketCycle = 0;

            // --- Decode complete packet ---
            g_LeftBtn   = (g_Packet[0] & 0x01) != 0;
            g_RightBtn  = (g_Packet[0] & 0x02) != 0;
            g_MiddleBtn = (g_Packet[0] & 0x04) != 0;

            // 9-bit signed deltas: byte 1/2 are the low 8 bits, sign bit
            // is bit 4 (X) / bit 5 (Y) of byte 0.
            int16_t dx = g_Packet[1];
            if (g_Packet[0] & 0x10)
                dx |= (int16_t)0xFF00;

            int16_t dy = g_Packet[2];
            if (g_Packet[0] & 0x20)
                dy |= (int16_t)0xFF00;

            // Drop the packet if either overflow bit is set.
            if ((g_Packet[0] & 0xC0) == 0)
            {
                g_MouseX += dx;
                g_MouseY -= dy;         // PS/2 Y points up; screen Y points down

                if (g_MouseX < 0)  g_MouseX = 0;
                if (g_MouseX > 79) g_MouseX = 79;
                if (g_MouseY < 0)  g_MouseY = 0;
                if (g_MouseY > 24) g_MouseY = 24;
            }
            break;
        }
    }
}

// =====================================================
// Initialization
// =====================================================

void i686_Mouse_Initialize(void)
{
    // 1. Enable auxiliary (mouse) PS/2 channel.
    PS2_WaitWrite();
    i686_outb(PS2_COMMAND_PORT, 0xA8);

    // 2. Enable IRQ12 + mouse clock in the PS/2 controller config byte.
    PS2_WaitWrite();
    i686_outb(PS2_COMMAND_PORT, 0x20);      // command: "read config byte"
    PS2_WaitRead();
    uint8_t config = i686_inb(PS2_DATA_PORT);
    config |= 0x02;        // bit 1: enable IRQ12
    config &= ~0x20;       // bit 5: clear "disable mouse clock"
    PS2_WaitWrite();
    i686_outb(PS2_COMMAND_PORT, 0x60);      // command: "write config byte"
    PS2_WaitWrite();
    i686_outb(PS2_DATA_PORT, config);

    // 3. Set mouse defaults (ACK = 0xFA).
    Mouse_SendCommand(0xF6);
    Mouse_ReadResponse();

    // 4. Enable data reporting.
    Mouse_SendCommand(0xF4);
    Mouse_ReadResponse();

    // 5. Flush any stray bytes still in the output buffer.
    while (i686_inb(PS2_STATUS_PORT) & 0x01)
        i686_inb(PS2_DATA_PORT);

    // 6. Register the IRQ12 handler.
    i686_IRQ_RegisterHandler(12, Mouse_Handler);

    printf("Mouse: initialized\r\n");
}

MouseState i686_Mouse_GetState(void)
{
    MouseState state;
    state.x            = g_MouseX;
    state.y            = g_MouseY;
    state.leftButton   = g_LeftBtn;
    state.rightButton  = g_RightBtn;
    state.middleButton = g_MiddleBtn;
    return state;
}
