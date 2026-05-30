#include "keyboard.h"
#include "io.h"
#include "irq.h"
#include "stdint.h"
#include "stdio.h"

#define KEYBOARD_DATA_PORT      0x60
#define KEYBOARD_STATUS_PORT    0x64
#define KEYBOARD_BUFFER_SIZE    128

// =====================================================
// Scan Code Set 1 -> ASCII translation
// =====================================================

static const char g_ScanCodeLower[] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   // 0x00-0x0E
    '\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',        // 0x0F-0x1C
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'','`',              // 0x1D-0x29
    0,   '\\','z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',  0,               // 0x2A-0x36
    '*',  0,  ' ',                                                                  // 0x37-0x39
};

static const char g_ScanCodeUpper[] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t','Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',  0,
    '*',  0,  ' ',
};

// =====================================================
// Circular key buffer (single-producer IRQ, single-consumer kernel loop)
// =====================================================

static char g_KeyBuffer[KEYBOARD_BUFFER_SIZE];
static volatile uint32_t g_BufferHead = 0;
static volatile uint32_t g_BufferTail = 0;

static void Buffer_Push(char c)
{
    uint32_t next = (g_BufferHead + 1) % KEYBOARD_BUFFER_SIZE;
    if (next != g_BufferTail)       // not full
    {
        g_KeyBuffer[g_BufferHead] = c;
        g_BufferHead = next;
    }
}

// =====================================================
// Modifier state
// =====================================================

static bool g_ShiftPressed = false;
static bool g_CtrlPressed  = false;
static bool g_CapsLock     = false;

// =====================================================
// IRQ1 handler
// =====================================================

static void Keyboard_Handler(Registers* regs)
{
    (void)regs;
    uint8_t scancode = i686_inb(KEYBOARD_DATA_PORT);

    // --- Key RELEASE (bit 7 set) ---
    if (scancode & 0x80)
    {
        uint8_t released = scancode & 0x7F;
        if (released == 0x2A || released == 0x36)   // shift released
            g_ShiftPressed = false;
        if (released == 0x1D)                       // left ctrl released
            g_CtrlPressed = false;
        return;
    }

    // --- Key PRESS ---
    switch (scancode)
    {
        case 0x2A:  // left shift
        case 0x36:  // right shift
            g_ShiftPressed = true;
            return;

        case 0x1D:  // left ctrl
            g_CtrlPressed = true;
            return;

        case 0x3A:  // caps lock toggle (single-shot on press)
            g_CapsLock = !g_CapsLock;
            return;
    }

    // Translate to ASCII
    if (scancode < sizeof(g_ScanCodeLower))
    {
        char lower = g_ScanCodeLower[scancode];
        char upper = g_ScanCodeUpper[scancode];
        char c;

        // Ctrl+letter emits the matching ASCII control code (Ctrl+A=0x01..Ctrl+Z=0x1A).
        // Specifically Ctrl+C = 0x03 (ETX) - used by the shell's 'multi' command.
        if (g_CtrlPressed && lower >= 'a' && lower <= 'z')
            c = (char)(lower - 'a' + 1);
        else if (g_ShiftPressed)
            c = upper;
        else if (g_CapsLock && lower >= 'a' && lower <= 'z')
            c = upper;      // caps lock only affects letters
        else
            c = lower;

        if (c != 0)
            Buffer_Push(c);
    }
}

// =====================================================
// Public API
// =====================================================

void i686_Keyboard_Initialize()
{
    i686_IRQ_RegisterHandler(1, Keyboard_Handler);
}

bool i686_Keyboard_HasKey()
{
    return g_BufferHead != g_BufferTail;
}

char i686_Keyboard_GetChar()
{
    while (!i686_Keyboard_HasKey())     // blocking wait
        ;

    char c = g_KeyBuffer[g_BufferTail];
    g_BufferTail = (g_BufferTail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}
