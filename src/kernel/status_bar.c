#include "status_bar.h"
#include "stdint.h"
#include <stdbool.h>

#define VGA_BUFFER  ((volatile uint8_t*)0xB8000)
#define VGA_COLS    80
#define COLOR_BAR   0x1F     // white on blue
#define COLOR_SEP   0x08     // dark gray on black

static volatile bool g_Ready = false;

static inline void put_cell(int x, int y, char c, uint8_t color)
{
    VGA_BUFFER[2 * (y * VGA_COLS + x)]     = (uint8_t)c;
    VGA_BUFFER[2 * (y * VGA_COLS + x) + 1] = color;
}

// Paints row 0 as "[ dhokaOS  uptime HH:MM:SS.cs ]" white-on-blue,
// padded to full width so trailing cells stay the bar background color.
static void paint_status(uint64_t ticks)
{
    // ticks at 100 Hz: 1 tick = 1 centisecond
    uint32_t total_cs = (uint32_t)ticks;
    uint32_t total_s  = total_cs / 100;
    uint32_t cs = total_cs % 100;
    uint32_t s  = total_s % 60;
    uint32_t m  = (total_s / 60) % 60;
    uint32_t h  = (total_s / 3600) % 100;   // wrap at 99h, plenty

    char buf[VGA_COLS];
    int  pos = 0;

    const char* prefix = "[ dhokaOS  uptime ";
    while (*prefix) buf[pos++] = *prefix++;

    buf[pos++] = '0' + (h  / 10);
    buf[pos++] = '0' + (h  % 10);
    buf[pos++] = ':';
    buf[pos++] = '0' + (m  / 10);
    buf[pos++] = '0' + (m  % 10);
    buf[pos++] = ':';
    buf[pos++] = '0' + (s  / 10);
    buf[pos++] = '0' + (s  % 10);
    buf[pos++] = '.';
    buf[pos++] = '0' + (cs / 10);
    buf[pos++] = '0' + (cs % 10);

    const char* suffix = " ]";
    while (*suffix) buf[pos++] = *suffix++;

    while (pos < VGA_COLS) buf[pos++] = ' ';   // pad rest of bar

    for (int x = 0; x < VGA_COLS; x++)
        put_cell(x, 0, buf[x], COLOR_BAR);
}

static void paint_separator(void)
{
    // 0xC4 is the single horizontal line in VGA code page 437.
    for (int x = 0; x < VGA_COLS; x++)
        put_cell(x, 1, (char)0xC4, COLOR_SEP);
}

void StatusBar_Initialize(void)
{
    paint_separator();
    paint_status(0);
    g_Ready = true;
}

void StatusBar_OnTick(uint64_t ticks)
{
    if (g_Ready)
        paint_status(ticks);
}
