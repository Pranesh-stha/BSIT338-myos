#include "snake.h"
#include "stdio.h"
#include "stdint.h"
#include <stdbool.h>
#include "arch/i686/keyboard.h"
#include "arch/i686/pit.h"

// =====================================================
// Geometry: status bar owns rows 0-1, score line is row 2,
// walls box rows 3..24, play area is the interior 4..23 x 1..78.
// =====================================================
#define SCORE_ROW    2
#define WALL_TOP     3
#define WALL_BOTTOM  24
#define WALL_LEFT    0
#define WALL_RIGHT   79
#define PLAY_TOP     4
#define PLAY_BOTTOM  23
#define PLAY_LEFT    1
#define PLAY_RIGHT   78

#define MAX_SNAKE    2048
#define CTRL_C       0x03

typedef enum { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT } Direction;
typedef struct { int8_t x, y; } Cell;

// =====================================================
// Glyphs (CP437 graphics for the body/food/walls)
// =====================================================
#define SNAKE_HEAD_CH   0xDB    // full block
#define SNAKE_BODY_CH   0xDB
#define FOOD_CH         0x04    // diamond
#define WALL_H_CH       0xCD
#define WALL_V_CH       0xBA
#define WALL_TL_CH      0xC9
#define WALL_TR_CH      0xBB
#define WALL_BL_CH      0xC8
#define WALL_BR_CH      0xBC

#define COL_SNAKE_BODY  VGA_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK)
#define COL_SNAKE_HEAD  VGA_COLOR(COLOR_WHITE,       COLOR_BLACK)
#define COL_FOOD        VGA_COLOR(COLOR_LIGHT_RED,   COLOR_BLACK)
#define COL_WALL        VGA_COLOR(COLOR_LIGHT_CYAN,  COLOR_BLACK)
#define COL_SCORE       VGA_COLOR(COLOR_YELLOW,      COLOR_BLACK)
#define COL_BG          VGA_COLOR(COLOR_BLACK,       COLOR_BLACK)

// =====================================================
// State
// =====================================================
static Cell      g_Snake[MAX_SNAKE];   // ring buffer of body cells
static int       g_Head, g_Tail;       // indices into ring; head = newest
static Direction g_Dir;
static Cell      g_Food;
static int       g_Score;
static uint32_t  g_RandState = 1;

// =====================================================
// PRNG (same LCG as the matrix rain)
// =====================================================
static uint32_t rand_next(void)
{
    g_RandState = g_RandState * 1103515245u + 12345u;
    return g_RandState;
}

// =====================================================
// Snake-cell membership test (used by food placement + collision)
// =====================================================
static bool snake_contains(int x, int y)
{
    int i = g_Tail;
    for (;;)
    {
        if (g_Snake[i].x == x && g_Snake[i].y == y) return true;
        if (i == g_Head) break;
        i = (i + 1) % MAX_SNAKE;
    }
    return false;
}

// =====================================================
// Rendering helpers
// =====================================================

static void clear_area(void)
{
    // Clears rows SCORE_ROW..24 so we return a tidy screen to the shell.
    for (int y = SCORE_ROW; y <= 24; y++)
        for (int x = 0; x < 80; x++)
            vga_put_cell(x, y, ' ', COL_BG);
}

static void draw_walls(void)
{
    for (int x = WALL_LEFT + 1; x < WALL_RIGHT; x++) {
        vga_put_cell(x, WALL_TOP,    WALL_H_CH, COL_WALL);
        vga_put_cell(x, WALL_BOTTOM, WALL_H_CH, COL_WALL);
    }
    for (int y = WALL_TOP + 1; y < WALL_BOTTOM; y++) {
        vga_put_cell(WALL_LEFT,  y, WALL_V_CH, COL_WALL);
        vga_put_cell(WALL_RIGHT, y, WALL_V_CH, COL_WALL);
    }
    vga_put_cell(WALL_LEFT,  WALL_TOP,    WALL_TL_CH, COL_WALL);
    vga_put_cell(WALL_RIGHT, WALL_TOP,    WALL_TR_CH, COL_WALL);
    vga_put_cell(WALL_LEFT,  WALL_BOTTOM, WALL_BL_CH, COL_WALL);
    vga_put_cell(WALL_RIGHT, WALL_BOTTOM, WALL_BR_CH, COL_WALL);
}

static void render_text(int x, int y, const char* str, uint8_t color)
{
    while (*str)
        vga_put_cell(x++, y, *str++, color);
}

static int u_to_str(unsigned n, char* out)
{
    // unsigned -> decimal, writes string + returns length
    char tmp[12];
    int  t = 0;
    if (n == 0) tmp[t++] = '0';
    while (n > 0) { tmp[t++] = '0' + (n % 10); n /= 10; }
    for (int i = 0; i < t; i++) out[i] = tmp[t - 1 - i];
    out[t] = 0;
    return t;
}

static void draw_score(void)
{
    // Clear score row
    for (int x = 0; x < 80; x++)
        vga_put_cell(x, SCORE_ROW, ' ', COL_SCORE);

    char num[12];
    u_to_str((unsigned)g_Score, num);

    int x = 2;
    render_text(x, SCORE_ROW, "Score: ", COL_SCORE);
    x += 7;
    render_text(x, SCORE_ROW, num, COL_SCORE);
    x += 6;
    render_text(x, SCORE_ROW, "  WASD: move    Ctrl+C: quit", COL_SCORE);
}

static void place_food(void)
{
    do {
        g_Food.x = PLAY_LEFT + (rand_next() % (PLAY_RIGHT - PLAY_LEFT + 1));
        g_Food.y = PLAY_TOP  + (rand_next() % (PLAY_BOTTOM - PLAY_TOP + 1));
    } while (snake_contains(g_Food.x, g_Food.y));

    vga_put_cell(g_Food.x, g_Food.y, FOOD_CH, COL_FOOD);
}

static void game_over_screen(void)
{
    // 30x9 centered box around (40, 14)
    int bx = 25, by = 10;   // top-left

    const char* L = "+----------------------------+";
    const char* M = "|                            |";
    render_text(bx, by + 0, L, VGA_COLOR(COLOR_LIGHT_RED, COLOR_BLACK));
    render_text(bx, by + 1, M, VGA_COLOR(COLOR_LIGHT_RED, COLOR_BLACK));
    render_text(bx, by + 2, "|        GAME  OVER          |",
                VGA_COLOR(COLOR_LIGHT_RED, COLOR_BLACK));
    render_text(bx, by + 3, M, VGA_COLOR(COLOR_LIGHT_RED, COLOR_BLACK));
    render_text(bx, by + 4, "|       Final score:         |",
                VGA_COLOR(COLOR_LIGHT_RED, COLOR_BLACK));
    render_text(bx, by + 5, M, VGA_COLOR(COLOR_LIGHT_RED, COLOR_BLACK));
    render_text(bx, by + 6, "|   Press any key to quit    |",
                VGA_COLOR(COLOR_LIGHT_RED, COLOR_BLACK));
    render_text(bx, by + 7, M, VGA_COLOR(COLOR_LIGHT_RED, COLOR_BLACK));
    render_text(bx, by + 8, L, VGA_COLOR(COLOR_LIGHT_RED, COLOR_BLACK));

    // Print score in the empty row 5 (inside the "Final score:" line)
    char num[12];
    int len = u_to_str((unsigned)g_Score, num);
    int sx = bx + 14 - (len / 2);   // visually center
    render_text(sx, by + 5, num, VGA_COLOR(COLOR_WHITE, COLOR_BLACK));

    // Drain any queued keys, then block until any key.
    while (i686_Keyboard_HasKey())
        i686_Keyboard_GetChar();
    while (!i686_Keyboard_HasKey())
        ;
    i686_Keyboard_GetChar();
}

// =====================================================
// Main game loop
// =====================================================
void Snake_Run(void)
{
    g_RandState = (uint32_t)i686_PIT_GetTicks() + 1;

    clear_area();
    draw_walls();

    // Initial snake: 5 segments centered, moving right.
    g_Tail = 0;
    g_Head = 4;
    for (int i = 0; i < 5; i++)
    {
        g_Snake[i].x = 35 + i;
        g_Snake[i].y = 14;
        vga_put_cell(g_Snake[i].x, g_Snake[i].y,
                     (i == 4) ? SNAKE_HEAD_CH : SNAKE_BODY_CH,
                     (i == 4) ? COL_SNAKE_HEAD : COL_SNAKE_BODY);
    }
    g_Dir   = DIR_RIGHT;
    g_Score = 0;

    place_food();
    draw_score();

    int       sleep_ms = 150;
    Direction next_dir = g_Dir;

    for (;;)
    {
        // -------- Drain keyboard input ---------
        while (i686_Keyboard_HasKey())
        {
            char c = i686_Keyboard_GetChar();
            if (c == CTRL_C)
            {
                clear_area();       // immediate quit, no game-over screen
                return;
            }
            if (c >= 'A' && c <= 'Z') c = (char)(c + 32);
            switch (c) {
                case 'w': if (g_Dir != DIR_DOWN)  next_dir = DIR_UP;    break;
                case 's': if (g_Dir != DIR_UP)    next_dir = DIR_DOWN;  break;
                case 'a': if (g_Dir != DIR_RIGHT) next_dir = DIR_LEFT;  break;
                case 'd': if (g_Dir != DIR_LEFT)  next_dir = DIR_RIGHT; break;
            }
        }
        g_Dir = next_dir;

        // -------- Compute new head ---------
        Cell oldHead = g_Snake[g_Head];
        Cell newHead = oldHead;
        switch (g_Dir) {
            case DIR_UP:    newHead.y--; break;
            case DIR_DOWN:  newHead.y++; break;
            case DIR_LEFT:  newHead.x--; break;
            case DIR_RIGHT: newHead.x++; break;
        }

        // Wall collision
        if (newHead.x < PLAY_LEFT || newHead.x > PLAY_RIGHT ||
            newHead.y < PLAY_TOP  || newHead.y > PLAY_BOTTOM)
        {
            i686_PIT_Sleep(300);
            game_over_screen();
            clear_area();
            return;
        }

        bool ate = (newHead.x == g_Food.x && newHead.y == g_Food.y);

        // Self collision (skip current tail if not eating - it'll move away)
        {
            int i = g_Tail;
            int skip_tail = ate ? -1 : g_Tail;
            bool crashed = false;
            for (;;)
            {
                if (i != skip_tail &&
                    g_Snake[i].x == newHead.x && g_Snake[i].y == newHead.y)
                { crashed = true; break; }
                if (i == g_Head) break;
                i = (i + 1) % MAX_SNAKE;
            }
            if (crashed)
            {
                i686_PIT_Sleep(300);
                game_over_screen();
                clear_area();
                return;
            }
        }

        // -------- Apply move ---------
        // Demote old head to body color, then paint new head.
        vga_set_color(oldHead.x, oldHead.y, COL_SNAKE_BODY);

        g_Head = (g_Head + 1) % MAX_SNAKE;
        g_Snake[g_Head] = newHead;
        vga_put_cell(newHead.x, newHead.y, SNAKE_HEAD_CH, COL_SNAKE_HEAD);

        if (ate)
        {
            g_Score++;
            place_food();
            draw_score();
            if (sleep_ms > 60) sleep_ms -= 4;   // gradually speed up
        }
        else
        {
            // Erase tail cell and advance tail pointer
            vga_put_cell(g_Snake[g_Tail].x, g_Snake[g_Tail].y, ' ', COL_BG);
            g_Tail = (g_Tail + 1) % MAX_SNAKE;
        }

        i686_PIT_Sleep(sleep_ms);
    }
}
