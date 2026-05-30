#include "stdio.h"
#include "arch/i686/io.h"
#include <stdarg.h>
#include <stdbool.h>

// ============================================================
// VGA text mode constants
// ============================================================
#define SCREEN_WIDTH  80
#define SCREEN_HEIGHT 25
#define DEFAULT_COLOR 0x07          // light gray on black

static uint8_t* g_ScreenBuffer = (uint8_t*)0xB8000;
static int      g_ScreenX     = 0;
static int      g_ScreenY     = 0;


// ============================================================
// Low-level cell access
// ============================================================
static void putchr(int x, int y, char c)
{
    g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x)] = c;
}

static void putcolor(int x, int y, uint8_t color)
{
    g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x) + 1] = color;
}

static char getchr(int x, int y)
{
    return g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x)];
}

static uint8_t getcolor(int x, int y)
{
    return g_ScreenBuffer[2 * (y * SCREEN_WIDTH + x) + 1];
}


// ============================================================
// Hardware cursor (write to VGA CRT controller via ports 0x3D4/0x3D5)
// ============================================================
static void setcursor(int x, int y)
{
    int pos = y * SCREEN_WIDTH + x;

    // Set low byte of cursor position
    i686_outb(0x3D4, 0x0F);
    i686_outb(0x3D5, (uint8_t)(pos & 0xFF));

    // Set high byte of cursor position
    i686_outb(0x3D4, 0x0E);
    i686_outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}


// ============================================================
// Clear screen and scroll
// ============================================================
void clrscr(void)
{
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            putchr(x, y, '\0');
            putcolor(x, y, DEFAULT_COLOR);
        }
    }
    g_ScreenX = 0;
    g_ScreenY = 0;
    setcursor(g_ScreenX, g_ScreenY);
}

void scrollback(int lines)
{
    // Move everything up by `lines` rows
    for (int y = lines; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            putchr(x, y - lines, getchr(x, y));
            putcolor(x, y - lines, getcolor(x, y));
        }
    }

    // Clear the last `lines` rows
    for (int y = SCREEN_HEIGHT - lines; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            putchr(x, y, '\0');
            putcolor(x, y, DEFAULT_COLOR);
        }
    }

    g_ScreenY -= lines;
}


// ============================================================
// putc — handles special characters, wrap, scroll
// ============================================================
void putc(char c)
{
    switch (c) {
        case '\n':
            g_ScreenX = 0;
            g_ScreenY++;
            break;

        case '\r':
            g_ScreenX = 0;
            break;

        case '\t':
            for (int i = 0; i < 4 - (g_ScreenX % 4); i++)
                putc(' ');
            break;

        default:
            putchr(g_ScreenX, g_ScreenY, c);
            g_ScreenX++;
            break;
    }

    if (g_ScreenX >= SCREEN_WIDTH) {
        g_ScreenY++;
        g_ScreenX = 0;
    }

    if (g_ScreenY >= SCREEN_HEIGHT) {
        scrollback(1);
    }

    setcursor(g_ScreenX, g_ScreenY);
}


void puts(const char* str)
{
    while (*str) {
        putc(*str);
        str++;
    }
}


// ============================================================
// printf state machine
// ============================================================
#define PRINTF_STATE_NORMAL         0
#define PRINTF_STATE_LENGTH         1
#define PRINTF_STATE_LENGTH_SHORT   2
#define PRINTF_STATE_LENGTH_LONG    3
#define PRINTF_STATE_SPEC           4

#define PRINTF_LENGTH_DEFAULT       0
#define PRINTF_LENGTH_SHORT_SHORT   1
#define PRINTF_LENGTH_SHORT         2
#define PRINTF_LENGTH_LONG          3
#define PRINTF_LENGTH_LONG_LONG     4


static const char g_HexChars[] = "0123456789abcdef";

static void printf_unsigned(unsigned long long number, int radix, int width, bool zero_pad)
{
    char buffer[32];
    int  pos = 0;

    // Convert to digits in reverse order
    do {
        unsigned long long rem = number % radix;
        number /= radix;
        buffer[pos++] = g_HexChars[rem];
    } while (number > 0);

    // Pad to width — append to buffer; printing happens top-down so
    // these end up as the leading characters of the output.
    char pad = zero_pad ? '0' : ' ';
    while (pos < width && pos < 32)
        buffer[pos++] = pad;

    // Print in correct order
    while (--pos >= 0)
        putc(buffer[pos]);
}

static void printf_signed(long long number, int radix, int width, bool zero_pad)
{
    bool negative = false;
    if (number < 0) {
        negative = true;
        number = -number;
    }

    char buffer[32];
    int  pos = 0;
    do {
        unsigned long long rem = (unsigned long long)number % radix;
        number /= radix;
        buffer[pos++] = g_HexChars[rem];
    } while (number > 0);

    int total_chars = pos + (negative ? 1 : 0);

    if (zero_pad) {
        // sign first, then zero padding, then digits: "-0042"
        if (negative) putc('-');
        while (total_chars < width) { putc('0'); total_chars++; }
        while (--pos >= 0) putc(buffer[pos]);
    } else {
        // spaces first, then sign, then digits: "  -42"
        while (total_chars < width) { putc(' '); total_chars++; }
        if (negative) putc('-');
        while (--pos >= 0) putc(buffer[pos]);
    }
}


void printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int state  = PRINTF_STATE_NORMAL;
    int length = PRINTF_LENGTH_DEFAULT;
    int radix  = 10;
    bool sign  = false;
    bool number = false;
    int  width = 0;
    bool zero_pad = false;

    while (*fmt) {
        switch (state) {
            case PRINTF_STATE_NORMAL:
                switch (*fmt) {
                    case '%':
                        state = PRINTF_STATE_LENGTH;
                        break;
                    default:
                        putc(*fmt);
                        break;
                }
                break;

            case PRINTF_STATE_LENGTH:
                // zero-pad flag: a leading '0' before any width digits
                if (*fmt == '0' && width == 0) {
                    zero_pad = true;
                    break;
                }
                // width digits
                if (*fmt >= '0' && *fmt <= '9') {
                    width = width * 10 + (*fmt - '0');
                    break;
                }
                switch (*fmt) {
                    case 'h':
                        length = PRINTF_LENGTH_SHORT;
                        state  = PRINTF_STATE_LENGTH_SHORT;
                        break;
                    case 'l':
                        length = PRINTF_LENGTH_LONG;
                        state  = PRINTF_STATE_LENGTH_LONG;
                        break;
                    default:
                        goto PRINTF_STATE_SPEC_;
                }
                break;

            case PRINTF_STATE_LENGTH_SHORT:
                if (*fmt == 'h') {
                    length = PRINTF_LENGTH_SHORT_SHORT;
                    state  = PRINTF_STATE_SPEC;
                } else {
                    goto PRINTF_STATE_SPEC_;
                }
                break;

            case PRINTF_STATE_LENGTH_LONG:
                if (*fmt == 'l') {
                    length = PRINTF_LENGTH_LONG_LONG;
                    state  = PRINTF_STATE_SPEC;
                } else {
                    goto PRINTF_STATE_SPEC_;
                }
                break;

            case PRINTF_STATE_SPEC:
            PRINTF_STATE_SPEC_:
                switch (*fmt) {
                    case 'c':
                        putc((char)va_arg(args, int));
                        break;

                    case 's':
                        puts(va_arg(args, const char*));
                        break;

                    case '%':
                        putc('%');
                        break;

                    case 'd':
                    case 'i':
                        radix = 10; sign = true; number = true;
                        break;

                    case 'u':
                        radix = 10; sign = false; number = true;
                        break;

                    case 'X':
                    case 'x':
                    case 'p':
                        radix = 16; sign = false; number = true;
                        break;

                    case 'o':
                        radix = 8;  sign = false; number = true;
                        break;

                    default:
                        break;          // unknown specifier — ignore
                }

                if (number) {
                    if (sign) {
                        switch (length) {
                            case PRINTF_LENGTH_SHORT_SHORT:
                            case PRINTF_LENGTH_SHORT:
                            case PRINTF_LENGTH_DEFAULT:
                                printf_signed(va_arg(args, int), radix, width, zero_pad);
                                break;
                            case PRINTF_LENGTH_LONG:
                                printf_signed(va_arg(args, long), radix, width, zero_pad);
                                break;
                            case PRINTF_LENGTH_LONG_LONG:
                                printf_signed(va_arg(args, long long), radix, width, zero_pad);
                                break;
                        }
                    } else {
                        switch (length) {
                            case PRINTF_LENGTH_SHORT_SHORT:
                            case PRINTF_LENGTH_SHORT:
                            case PRINTF_LENGTH_DEFAULT:
                                printf_unsigned(va_arg(args, unsigned int), radix, width, zero_pad);
                                break;
                            case PRINTF_LENGTH_LONG:
                                printf_unsigned(va_arg(args, unsigned long), radix, width, zero_pad);
                                break;
                            case PRINTF_LENGTH_LONG_LONG:
                                printf_unsigned(va_arg(args, unsigned long long), radix, width, zero_pad);
                                break;
                        }
                    }
                }

                // Reset state
                state  = PRINTF_STATE_NORMAL;
                length = PRINTF_LENGTH_DEFAULT;
                radix  = 10;
                sign   = false;
                number = false;
                width  = 0;
                zero_pad = false;
                break;
        }

        fmt++;
    }

    va_end(args);                       // ← bug nanobyte forgot in the stream
}


// ============================================================
// print_buffer — hex dump for debugging
// ============================================================
void print_buffer(const char* msg, const void* buffer, uint32_t count)
{
    const uint8_t* u8buffer = (const uint8_t*)buffer;

    puts(msg);
    for (uint16_t i = 0; i < count; i++) {
        putc(g_HexChars[u8buffer[i] >> 4]);
        putc(g_HexChars[u8buffer[i] & 0x0F]);
    }
    puts("\n");
}
