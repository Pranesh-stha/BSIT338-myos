#include "shell.h"
#include "stdint.h"
#include "stdio.h"
#include "arch/i686/keyboard.h"
#include "arch/i686/mouse.h"
#include "arch/i686/pit.h"
#include "pmm.h"
#include "scheduler.h"
#include "matrix.h"
#include "mandel.h"

#define LINE_MAX 128
#define CTRL_C   0x03

// Mouse cursor must avoid the status bar (rows 0-1). Same constant as
// SCREEN_TOP in stdio.c.
#define CURSOR_TOP_ROW 2

// Local strcmp.
static int streq(const char* a, const char* b)
{
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == 0 && *b == 0;
}

// =====================================================
// Multitasking demo tasks (used by `multi`)
// =====================================================

static void taskA(void) { for (;;) { printf("A"); i686_PIT_Sleep(500);  } }
static void taskB(void) { for (;;) { printf("B"); i686_PIT_Sleep(700);  } }
static void taskC(void) { for (;;) { printf("C"); i686_PIT_Sleep(1000); } }

// =====================================================
// Commands
// =====================================================

static void cmd_time(void)
{
    set_color(VGA_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
    printf("Running 5-second timer test...\r\n");
    for (int i = 0; i < 5; i++)
    {
        printf("  Tick %d (uptime: %llu ms)\r\n",
               i, i686_PIT_GetTicks() * 10);
        i686_PIT_Sleep(1000);
    }
    printf("Done.\r\n");
    set_color(VGA_DEFAULT);
}

static void cmd_mem(void)
{
    set_color(VGA_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
    PMM_PrintStats();
    set_color(VGA_DEFAULT);
}

static void cmd_multi(void)
{
    if (Scheduler_CreateTask(taskA) == NULL ||
        Scheduler_CreateTask(taskB) == NULL ||
        Scheduler_CreateTask(taskC) == NULL)
    {
        set_color(VGA_COLOR(COLOR_LIGHT_RED, COLOR_BLACK));
        printf("Failed to allocate task stacks.\r\n");
        set_color(VGA_DEFAULT);
        Scheduler_StopAndReset();
        return;
    }

    set_color(VGA_COLOR(COLOR_YELLOW, COLOR_BLACK));
    printf("Multitasking demo: 3 background tasks running.\r\n");
    printf("Press Ctrl+C to stop.\r\n");
    set_color(VGA_DEFAULT);

    Scheduler_Start();

    for (;;)
    {
        if (i686_Keyboard_HasKey())
        {
            char c = i686_Keyboard_GetChar();
            if (c == CTRL_C) break;
        }
    }

    Scheduler_StopAndReset();

    while (i686_Keyboard_HasKey())
        i686_Keyboard_GetChar();

    set_color(VGA_COLOR(COLOR_YELLOW, COLOR_BLACK));
    printf("\r\n^C  Demo stopped.\r\n");
    set_color(VGA_DEFAULT);
}

static void cmd_matrix(void)
{
    set_color(VGA_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
    printf("Matrix rain - press Ctrl+C to stop.\r\n");
    set_color(VGA_DEFAULT);
    i686_PIT_Sleep(700);     // brief pause so user sees the message

    Matrix_Run();             // blocks until Ctrl+C; restores text area on exit

    while (i686_Keyboard_HasKey())
        i686_Keyboard_GetChar();

    set_color(VGA_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
    printf("\r\n^C  Matrix stopped.\r\n");
    set_color(VGA_DEFAULT);
}

static void cmd_mandel(void)
{
    set_color(VGA_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
    printf("Rendering Mandelbrot set (fixed-point math)...\r\n");
    set_color(VGA_DEFAULT);

    Mandel_Render();          // writes directly to VGA cells, no scroll

    // After render, the text area is full of the fractal; wait for any
    // keypress, then clear and return to prompt.
    while (!i686_Keyboard_HasKey())
        ;
    i686_Keyboard_GetChar();
    clrscr();
    set_color(VGA_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
    printf("Rendered.\r\n");
    set_color(VGA_DEFAULT);
}

// ---- mouse cursor helpers ----

static int     g_CursorLastX     = -1;
static int     g_CursorLastY     = -1;
static uint8_t g_CursorSavedColor = VGA_DEFAULT;
#define CURSOR_COLOR VGA_COLOR(COLOR_BLACK, COLOR_WHITE)

static void cursor_clear(void)
{
    if (g_CursorLastX >= 0)
    {
        vga_set_color(g_CursorLastX, g_CursorLastY, g_CursorSavedColor);
        g_CursorLastX = -1;
    }
}

static void cursor_move(int x, int y)
{
    cursor_clear();
    if (y < CURSOR_TOP_ROW) return;     // don't overwrite status bar rows
    g_CursorSavedColor = vga_get_color(x, y);
    vga_set_color(x, y, CURSOR_COLOR);
    g_CursorLastX = x;
    g_CursorLastY = y;
}

static void cmd_mouse(void)
{
    set_color(VGA_COLOR(COLOR_YELLOW, COLOR_BLACK));
    printf("Move the mouse / click buttons. Ctrl+C to stop.\r\n");
    printf("(If cursor is stolen by QEMU, press Ctrl+Alt+G to release.)\r\n\r\n");
    set_color(VGA_DEFAULT);

    int32_t lastX = -1, lastY = -1;
    bool    lastL = false, lastR = false, lastM = false;

    // Initial paint
    MouseState init = i686_Mouse_GetState();
    cursor_move(init.x, init.y);

    for (;;)
    {
        if (i686_Keyboard_HasKey())
        {
            char c = i686_Keyboard_GetChar();
            if (c == CTRL_C) break;
        }

        MouseState ms = i686_Mouse_GetState();

        if (ms.x != lastX || ms.y != lastY ||
            ms.leftButton   != lastL ||
            ms.rightButton  != lastR ||
            ms.middleButton != lastM)
        {
            // Move the highlight cell (no-op if x/y unchanged)
            if (ms.x != lastX || ms.y != lastY)
                cursor_move(ms.x, ms.y);

            set_color(VGA_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
            printf("\rX:%3d Y:%3d  L:%d M:%d R:%d   ",
                   ms.x, ms.y,
                   ms.leftButton, ms.middleButton, ms.rightButton);
            set_color(VGA_DEFAULT);

            lastX = ms.x; lastY = ms.y;
            lastL = ms.leftButton;
            lastR = ms.rightButton;
            lastM = ms.middleButton;

            // The info line printf can repaint the cell currently under
            // the cursor (if it lands on the same row). Re-paint cursor
            // so the highlight comes back on top of the info text.
            cursor_move(ms.x, ms.y);
        }
    }

    cursor_clear();

    while (i686_Keyboard_HasKey())
        i686_Keyboard_GetChar();

    set_color(VGA_COLOR(COLOR_YELLOW, COLOR_BLACK));
    printf("\r\n^C  Mouse watch stopped.\r\n");
    set_color(VGA_DEFAULT);
}

static void cmd_help(void)
{
    set_color(VGA_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
    printf("Commands:\r\n");
    set_color(VGA_DEFAULT);
    printf("  time    - 5-second PIT timer test\r\n");
    printf("  mem     - print PMM stats\r\n");
    printf("  multi   - multitasking demo (Ctrl+C to stop)\r\n");
    printf("  mouse   - live mouse coords with on-screen cursor (Ctrl+C)\r\n");
    printf("  matrix  - falling-text screensaver (Ctrl+C to stop)\r\n");
    printf("  mandel  - render Mandelbrot set; press any key to dismiss\r\n");
    printf("  clear   - clear the screen\r\n");
    printf("  help    - this message\r\n");
}

static void exec(const char* line)
{
    if (line[0] == 0)            return;
    if (streq(line, "time"))   { cmd_time();   return; }
    if (streq(line, "mem"))    { cmd_mem();    return; }
    if (streq(line, "multi"))  { cmd_multi();  return; }
    if (streq(line, "mouse"))  { cmd_mouse();  return; }
    if (streq(line, "matrix")) { cmd_matrix(); return; }
    if (streq(line, "mandel")) { cmd_mandel(); return; }
    if (streq(line, "clear"))  { clrscr();     return; }
    if (streq(line, "help"))   { cmd_help();   return; }

    set_color(VGA_COLOR(COLOR_LIGHT_RED, COLOR_BLACK));
    printf("unknown command: %s  (type 'help')\r\n", line);
    set_color(VGA_DEFAULT);
}

// =====================================================
// Read-eval-print loop
// =====================================================

void Shell_Run(void)
{
    char line[LINE_MAX];
    int  pos;

    set_color(VGA_COLOR(COLOR_LIGHT_GRAY, COLOR_BLACK));
    printf("\r\nType 'help' for commands.\r\n");
    set_color(VGA_DEFAULT);

    for (;;)
    {
        // Cyan prompt
        set_color(VGA_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
        printf("> ");
        set_color(VGA_DEFAULT);

        pos = 0;

        for (;;)
        {
            char c = i686_Keyboard_GetChar();

            if (c == '\n')
            {
                printf("\r\n");
                line[pos] = 0;
                break;
            }
            if (c == '\b')
            {
                if (pos > 0)
                {
                    pos--;
                    printf("%c", '\b');
                }
                continue;
            }
            if (pos < LINE_MAX - 1)
            {
                line[pos++] = c;
                printf("%c", c);
            }
        }

        exec(line);
    }
}
