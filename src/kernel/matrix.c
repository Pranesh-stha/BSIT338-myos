#include "matrix.h"
#include "stdio.h"
#include "stdint.h"
#include "arch/i686/keyboard.h"
#include "arch/i686/pit.h"

#define MATRIX_TOP_ROW   2          // skip status bar (rows 0-1)
#define MATRIX_BOT_ROW   24         // last text row
#define MATRIX_ROWS      (MATRIX_BOT_ROW - MATRIX_TOP_ROW + 1)
#define MATRIX_COLS      80
#define TRAIL_LEN        6
#define FRAME_DELAY_MS   60

// Simple LCG PRNG. Seeded from PIT tick count at start.
static uint32_t g_RandState = 1;
static uint32_t rand_next(void)
{
    g_RandState = g_RandState * 1103515245u + 12345u;
    return g_RandState;
}

// Per-column head row position (within MATRIX_ROWS) and tick countdown
// so columns advance at different speeds.
static int g_Head[MATRIX_COLS];
static int g_Speed[MATRIX_COLS];
static int g_Counter[MATRIX_COLS];

static char rand_glyph(void)
{
    uint32_t r = rand_next();
    // mix of letters, digits, and CP437 graphics chars
    static const char glyphs[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
        "!@#$%^&*()-+=<>/?";
    return glyphs[r % (sizeof(glyphs) - 1)];
}

static void clear_text_area(void)
{
    for (int y = MATRIX_TOP_ROW; y <= MATRIX_BOT_ROW; y++)
        for (int x = 0; x < MATRIX_COLS; x++)
            vga_put_cell(x, y, ' ', VGA_DEFAULT);
}

void Matrix_Run(void)
{
    // Seed PRNG with current ticks so each run is different
    g_RandState = (uint32_t)i686_PIT_GetTicks() + 1;

    for (int c = 0; c < MATRIX_COLS; c++)
    {
        g_Head[c]    = -(int)(rand_next() % MATRIX_ROWS);   // staggered start
        g_Speed[c]   = 1 + (rand_next() % 4);               // 1..4 frames per step
        g_Counter[c] = 0;
    }

    clear_text_area();

    for (;;)
    {
        if (i686_Keyboard_HasKey())
        {
            char k = i686_Keyboard_GetChar();
            if (k == 0x03) break;       // Ctrl+C
        }

        // Advance each column
        for (int c = 0; c < MATRIX_COLS; c++)
        {
            if (++g_Counter[c] < g_Speed[c]) continue;
            g_Counter[c] = 0;

            int head = g_Head[c];

            // Erase the cell that fell off the tail
            int tail = head - TRAIL_LEN;
            if (tail >= 0 && tail < MATRIX_ROWS)
            {
                vga_put_cell(c, MATRIX_TOP_ROW + tail, ' ',
                             VGA_COLOR(COLOR_BLACK, COLOR_BLACK));
            }

            // Dim the previous head to dark green
            if (head - 1 >= 0 && head - 1 < MATRIX_ROWS)
            {
                uint8_t color = VGA_COLOR(COLOR_GREEN, COLOR_BLACK);
                vga_set_color(c, MATRIX_TOP_ROW + head - 1, color);
            }

            // Paint a new bright-green head glyph at the new position
            if (head >= 0 && head < MATRIX_ROWS)
            {
                vga_put_cell(c, MATRIX_TOP_ROW + head,
                             rand_glyph(),
                             VGA_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
            }

            // Advance head; respawn at top once past bottom
            g_Head[c]++;
            if (g_Head[c] >= MATRIX_ROWS + TRAIL_LEN)
            {
                g_Head[c]    = -(int)(rand_next() % 8);
                g_Speed[c]   = 1 + (rand_next() % 4);
            }
        }

        i686_PIT_Sleep(FRAME_DELAY_MS);
    }

    clear_text_area();
}
