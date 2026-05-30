#include "shell.h"
#include "stdint.h"
#include "stdio.h"
#include "arch/i686/io.h"
#include "arch/i686/keyboard.h"
#include "arch/i686/mouse.h"
#include "arch/i686/pit.h"
#include "arch/i686/speaker.h"
#include "pmm.h"
#include "scheduler.h"
#include "matrix.h"
#include "mandel.h"
#include "snake.h"
#include "expr.h"

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

// A line "looks like math" if its first non-whitespace char is a digit,
// '(', or a leading +/-. Anything else (letters etc.) is a command.
static int looks_like_math(const char* s)
{
    while (*s == ' ' || *s == '\t') s++;
    if (*s == 0) return 0;
    if (*s == '(' || *s == '-' || *s == '+') return 1;
    if (*s >= '0' && *s <= '9') return 1;
    return 0;
}

static void try_math(const char* line)
{
    int result;
    ExprError err = Expr_Eval(line, &result);

    if (err == EXPR_OK)
    {
        set_color(VGA_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
        printf("= %d\r\n", result);
        set_color(VGA_DEFAULT);
        return;
    }

    set_color(VGA_COLOR(COLOR_LIGHT_RED, COLOR_BLACK));
    if (err == EXPR_DIV_ZERO)
        printf("math error: division by zero\r\n");
    else
        printf("math error: invalid expression\r\n");
    set_color(VGA_DEFAULT);
}

// =====================================================
// Command history (ring buffer of recent lines)
// =====================================================

#define HISTORY_SIZE 16

static char g_History[HISTORY_SIZE][LINE_MAX];
static int  g_HistCount = 0;       // number of entries stored (<= HISTORY_SIZE)
static int  g_HistHead  = 0;       // next-write index; (head-1) = newest

static void history_save(const char* line)
{
    if (line[0] == 0) return;       // skip empties

    // Skip if same as last (no duplicate adjacents).
    if (g_HistCount > 0)
    {
        int last = (g_HistHead - 1 + HISTORY_SIZE) % HISTORY_SIZE;
        if (streq(g_History[last], line)) return;
    }

    int i = 0;
    while (line[i] && i < LINE_MAX - 1)
    {
        g_History[g_HistHead][i] = line[i];
        i++;
    }
    g_History[g_HistHead][i] = 0;

    g_HistHead = (g_HistHead + 1) % HISTORY_SIZE;
    if (g_HistCount < HISTORY_SIZE) g_HistCount++;
}

// offset 0 = newest entry, offset (count-1) = oldest. NULL if out of range.
static const char* history_get(int offset)
{
    if (offset < 0 || offset >= g_HistCount) return NULL;
    int idx = (g_HistHead - 1 - offset + HISTORY_SIZE * 2) % HISTORY_SIZE;
    return g_History[idx];
}

// Erase whatever is currently echoed for the line buffer and replace it
// (both in line[] and on screen) with `new_line`. Updates *pos.
static void replace_line(char line[LINE_MAX], int* pos, const char* new_line)
{
    for (int i = 0; i < *pos; i++) printf("%c", '\b');

    *pos = 0;
    if (new_line == NULL) { line[0] = 0; return; }

    while (new_line[*pos] && *pos < LINE_MAX - 1)
    {
        line[*pos] = new_line[*pos];
        printf("%c", new_line[*pos]);
        (*pos)++;
    }
    line[*pos] = 0;
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

// Returns 1 if `s` begins with `prefix`; same arg list as strncmp != 0.
static int starts_with(const char* s, const char* prefix)
{
    while (*prefix)
    {
        if (*s != *prefix) return 0;
        s++; prefix++;
    }
    return 1;
}

static void cmd_timer(int total_sec)
{
    set_color(VGA_COLOR(COLOR_YELLOW, COLOR_BLACK));
    printf("Timer: %d seconds. Ctrl+C to cancel.\r\n", total_sec);
    set_color(VGA_DEFAULT);

    for (int remaining = total_sec; remaining > 0; remaining--)
    {
        int mins = remaining / 60;
        int secs = remaining % 60;

        set_color(VGA_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
        printf("\r  %02d:%02d remaining...   ", mins, secs);
        set_color(VGA_DEFAULT);

        // Sleep 1 second in 100 ms slices so Ctrl+C is responsive.
        for (int i = 0; i < 10; i++)
        {
            i686_PIT_Sleep(100);
            if (i686_Keyboard_HasKey())
            {
                char c = i686_Keyboard_GetChar();
                if (c == 0x03)
                {
                    set_color(VGA_COLOR(COLOR_YELLOW, COLOR_BLACK));
                    printf("\r\n^C  Timer cancelled.\r\n");
                    set_color(VGA_DEFAULT);
                    return;
                }
                // other keys: discard
            }
        }
    }

    set_color(VGA_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
    printf("\r  00:00 - DONE!                  \r\n");
    set_color(VGA_DEFAULT);

    // Alarm: 3 short beeps
    for (int i = 0; i < 3; i++)
    {
        Speaker_Beep(1000, 200);
        i686_PIT_Sleep(150);
    }
}

static void cmd_timer_usage(void)
{
    set_color(VGA_COLOR(COLOR_LIGHT_RED, COLOR_BLACK));
    printf("usage: timer <seconds>   (e.g. 'timer 30' or 'timer 5*60')\r\n");
    set_color(VGA_DEFAULT);
}

static void cmd_beep(void)
{
    set_color(VGA_COLOR(COLOR_YELLOW, COLOR_BLACK));
    printf("beep!\r\n");
    set_color(VGA_DEFAULT);
    Speaker_Beep(880, 150);     // 880 Hz (A5) for 150 ms
}

static void cmd_melody(void)
{
    set_color(VGA_COLOR(COLOR_YELLOW, COLOR_BLACK));
    printf("Playing melody (no audio? See README for QEMU sound flags.)\r\n");
    set_color(VGA_DEFAULT);
    Speaker_PlayMelody();
}

static void cmd_snake(void)
{
    set_color(VGA_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
    printf("Starting snake. WASD to move, Ctrl+C to quit.\r\n");
    set_color(VGA_DEFAULT);
    i686_PIT_Sleep(400);

    Snake_Run();

    while (i686_Keyboard_HasKey())
        i686_Keyboard_GetChar();

    clrscr();
    set_color(VGA_COLOR(COLOR_LIGHT_GREEN, COLOR_BLACK));
    printf("Snake exited.\r\n");
    set_color(VGA_DEFAULT);
}

static void cmd_quit(void)
{
    set_color(VGA_COLOR(COLOR_YELLOW, COLOR_BLACK));
    printf("Shutting down dhokaOS...\r\n");
    set_color(VGA_DEFAULT);

    // Brief beep so the user gets feedback even if the screen vanishes.
    Speaker_Beep(660, 120);
    i686_PIT_Sleep(120);

    // ACPI shutdown ports for common hypervisors. The host kills the VM
    // immediately on a successful write; whichever one works first wins.
    i686_outw(0x0604, 0x2000);   // QEMU 2.0+
    i686_outw(0xB004, 0x2000);   // Bochs / older QEMU
    i686_outw(0x4004, 0x3400);   // VirtualBox

    // If we're still here, the host didn't accept any of those. Halt the
    // CPU forever so we don't return to a confused shell.
    i686_DisableInterrupts();
    for (;;) __asm__ volatile ("hlt");
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
    printf("  snake   - play snake (WASD to move, Ctrl+C to quit)\r\n");
    printf("  beep    - short PC-speaker beep\r\n");
    printf("  melody  - play a short tune through the PC speaker\r\n");
    printf("  timer N - countdown for N seconds, then 3 alarm beeps\r\n");
    printf("              N can be an expression (e.g. timer 5*60)\r\n");
    printf("  clear   - clear the screen\r\n");
    printf("  help    - this message\r\n");
    printf("  quit    - shut down (also: exit, shutdown)\r\n");
    printf("\r\n");
    printf("Math: type any expression to evaluate. Supports +-*/%% and ().\r\n");
    printf("Examples:  6-1   |   (5+9)-6   |   2+3*4   |   100/3   |   17%%5\r\n");
    printf("\r\n");
    printf("Line editing: use Up/Down arrows to recall previous commands.\r\n");
}

static void exec(const char* line)
{
    if (line[0] == 0)            return;

    // Math first - if it parses as an expression, evaluate and print.
    if (looks_like_math(line)) { try_math(line);  return; }

    // Commands that take args go before the bare-word matches.
    if (starts_with(line, "timer "))
    {
        const char* arg = line + 6;
        while (*arg == ' ') arg++;
        int sec;
        if (Expr_Eval(arg, &sec) != EXPR_OK || sec <= 0) { cmd_timer_usage(); return; }
        cmd_timer(sec);
        return;
    }
    if (streq(line, "timer"))  { cmd_timer_usage(); return; }

    if (streq(line, "time"))   { cmd_time();   return; }
    if (streq(line, "mem"))    { cmd_mem();    return; }
    if (streq(line, "multi"))  { cmd_multi();  return; }
    if (streq(line, "mouse"))  { cmd_mouse();  return; }
    if (streq(line, "matrix")) { cmd_matrix(); return; }
    if (streq(line, "mandel")) { cmd_mandel(); return; }
    if (streq(line, "snake"))  { cmd_snake();  return; }
    if (streq(line, "beep"))   { cmd_beep();   return; }
    if (streq(line, "melody")) { cmd_melody(); return; }
    if (streq(line, "clear"))  { clrscr();     return; }
    if (streq(line, "help"))   { cmd_help();   return; }
    if (streq(line, "quit")     ||
        streq(line, "exit")     ||
        streq(line, "shutdown")) { cmd_quit(); return; }

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
        int hist_offset = -1;       // -1 = the user's own input, 0+ = history

        for (;;)
        {
            char c = i686_Keyboard_GetChar();

            if (c == '\n')
            {
                printf("\r\n");
                line[pos] = 0;
                history_save(line);
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
            if (c == KEY_UP)
            {
                if (hist_offset + 1 < g_HistCount)
                {
                    hist_offset++;
                    replace_line(line, &pos, history_get(hist_offset));
                }
                continue;
            }
            if (c == KEY_DOWN)
            {
                if (hist_offset > 0)
                {
                    hist_offset--;
                    replace_line(line, &pos, history_get(hist_offset));
                }
                else if (hist_offset == 0)
                {
                    hist_offset = -1;
                    replace_line(line, &pos, "");
                }
                continue;
            }
            if (c == KEY_LEFT || c == KEY_RIGHT)
                continue;       // mid-line editing not implemented

            // Only accept printable characters into the buffer.
            if (pos < LINE_MAX - 1 && c >= 0x20 && c < 0x7F)
            {
                line[pos++] = c;
                printf("%c", c);
            }
        }

        exec(line);
    }
}
