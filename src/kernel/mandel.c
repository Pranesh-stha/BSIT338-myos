#include "mandel.h"
#include "stdio.h"
#include "stdint.h"

// =====================================================
// 16.16 fixed-point arithmetic. libgcc provides the 64-bit
// helpers we need for the multiply.
// =====================================================
#define FP_SHIFT 16
#define FP_ONE   (1 << FP_SHIFT)
#define FP(x)    ((int32_t)((x) * FP_ONE))

static inline int32_t fp_mul(int32_t a, int32_t b)
{
    return (int32_t)(((int64_t)a * (int64_t)b) >> FP_SHIFT);
}

#define MANDEL_TOP_ROW   2
#define MANDEL_BOT_ROW   24
#define MANDEL_ROWS      (MANDEL_BOT_ROW - MANDEL_TOP_ROW + 1)
#define MANDEL_COLS      80
#define MAX_ITER         50

// Density-mapped shades from least to most "in-set". Inside the set
// (didn't escape) gets the densest glyph.
static const char g_Shades[]  = " .:-=+*#%@";
#define NUM_SHADES (sizeof(g_Shades) - 1)

static uint8_t color_for_iter(int iter)
{
    if (iter >= MAX_ITER)
        return VGA_COLOR(COLOR_WHITE, COLOR_BLACK);     // inside the set

    // Outside: bands of color by iteration depth
    if (iter < 5)  return VGA_COLOR(COLOR_BLUE,         COLOR_BLACK);
    if (iter < 10) return VGA_COLOR(COLOR_LIGHT_BLUE,   COLOR_BLACK);
    if (iter < 18) return VGA_COLOR(COLOR_CYAN,         COLOR_BLACK);
    if (iter < 25) return VGA_COLOR(COLOR_LIGHT_CYAN,   COLOR_BLACK);
    if (iter < 35) return VGA_COLOR(COLOR_MAGENTA,      COLOR_BLACK);
    return                VGA_COLOR(COLOR_LIGHT_MAGENTA,COLOR_BLACK);
}

void Mandel_Render(void)
{
    const int32_t x_min = FP(-2.0);
    const int32_t x_max = FP( 1.0);
    const int32_t y_min = FP(-1.0);
    const int32_t y_max = FP( 1.0);
    const int32_t four  = FP(4.0);

    const int32_t dx = (x_max - x_min) / MANDEL_COLS;
    const int32_t dy = (y_max - y_min) / MANDEL_ROWS;

    for (int row = 0; row < MANDEL_ROWS; row++)
    {
        int32_t cy = y_min + dy * row;

        for (int col = 0; col < MANDEL_COLS; col++)
        {
            int32_t cx = x_min + dx * col;

            int32_t zx = 0, zy = 0;
            int iter;
            for (iter = 0; iter < MAX_ITER; iter++)
            {
                int32_t zx2 = fp_mul(zx, zx);
                int32_t zy2 = fp_mul(zy, zy);
                if (zx2 + zy2 > four) break;

                int32_t new_zx = zx2 - zy2 + cx;
                int32_t new_zy = (fp_mul(zx, zy) << 1) + cy;
                zx = new_zx;
                zy = new_zy;
            }

            int shade = (iter * (int)NUM_SHADES) / MAX_ITER;
            if (shade >= (int)NUM_SHADES) shade = (int)NUM_SHADES - 1;

            vga_put_cell(col, MANDEL_TOP_ROW + row,
                         g_Shades[shade],
                         color_for_iter(iter));
        }
    }
}
