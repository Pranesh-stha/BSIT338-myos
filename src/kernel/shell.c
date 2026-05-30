#include "shell.h"
#include "stdint.h"
#include "stdio.h"
#include "arch/i686/keyboard.h"
#include "arch/i686/mouse.h"
#include "arch/i686/pit.h"
#include "pmm.h"
#include "scheduler.h"

#define LINE_MAX 128

// Local strcmp — kernel doesn't have a string lib yet.
static int streq(const char* a, const char* b)
{
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == 0 && *b == 0;
}

// =====================================================
// Multitasking demo tasks. Each prints its letter at a different
// cadence so you can see preemptive scheduling interleave them.
// =====================================================

static void taskA(void)
{
    for (;;) { printf("A"); i686_PIT_Sleep(500); }
}

static void taskB(void)
{
    for (;;) { printf("B"); i686_PIT_Sleep(700); }
}

static void taskC(void)
{
    for (;;) { printf("C"); i686_PIT_Sleep(1000); }
}

// =====================================================
// Commands
// =====================================================

static void cmd_time(void)
{
    printf("Running 5-second timer test...\r\n");
    for (int i = 0; i < 5; i++)
    {
        printf("  Tick %d (uptime: %llu ms)\r\n",
               i, i686_PIT_GetTicks() * 10);
        i686_PIT_Sleep(1000);
    }
    printf("Done.\r\n");
}

static void cmd_mem(void)
{
    PMM_PrintStats();
}

#define CTRL_C   0x03   // ASCII ETX - what our keyboard driver emits for Ctrl+C

static void cmd_multi(void)
{
    if (Scheduler_CreateTask(taskA) == NULL ||
        Scheduler_CreateTask(taskB) == NULL ||
        Scheduler_CreateTask(taskC) == NULL)
    {
        printf("Failed to allocate task stacks.\r\n");
        Scheduler_StopAndReset();
        return;
    }

    printf("Multitasking demo: 3 background tasks running.\r\n");
    printf("Press Ctrl+C to stop.\r\n");

    Scheduler_Start();   // enables preemption; PIT will now switch tasks

    // Run forever until the user hits Ctrl+C. Other keystrokes are
    // silently consumed (no echo) so they don't muddle the demo output.
    for (;;)
    {
        if (i686_Keyboard_HasKey())
        {
            char c = i686_Keyboard_GetChar();
            if (c == CTRL_C) break;
        }
        // body otherwise idle; PIT preempts us to A/B/C every tick
    }

    Scheduler_StopAndReset();

    // Drain any remaining queued keys so they don't dump onto the next prompt.
    while (i686_Keyboard_HasKey())
        i686_Keyboard_GetChar();

    printf("\r\n^C  Demo stopped.\r\n");
}

static void cmd_mouse(void)
{
    printf("Move the mouse / click buttons. Ctrl+C to stop.\r\n");
    printf("(QEMU window must have focus - if cursor is stolen,\r\n");
    printf(" press Ctrl+Alt+G to release it back to the host.)\r\n\r\n");

    int32_t lastX = -1, lastY = -1;
    bool    lastL = false, lastR = false, lastM = false;

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
            printf("\rX:%3d Y:%3d  L:%d M:%d R:%d   ",
                   ms.x, ms.y,
                   ms.leftButton, ms.middleButton, ms.rightButton);

            lastX = ms.x; lastY = ms.y;
            lastL = ms.leftButton;
            lastR = ms.rightButton;
            lastM = ms.middleButton;
        }
    }

    while (i686_Keyboard_HasKey())
        i686_Keyboard_GetChar();

    printf("\r\n^C  Mouse watch stopped.\r\n");
}

static void cmd_help(void)
{
    printf("Commands:\r\n");
    printf("  time   - 5-second PIT timer test\r\n");
    printf("  mem    - print PMM stats\r\n");
    printf("  multi  - multitasking demo (A/B/C tasks, Ctrl+C to stop)\r\n");
    printf("  mouse  - live mouse coords + buttons (Ctrl+C to stop)\r\n");
    printf("  clear  - clear the screen\r\n");
    printf("  help   - this message\r\n");
}

static void exec(const char* line)
{
    if (line[0] == 0)            return;
    if (streq(line, "time"))   { cmd_time();  return; }
    if (streq(line, "mem"))    { cmd_mem();   return; }
    if (streq(line, "multi"))  { cmd_multi(); return; }
    if (streq(line, "mouse"))  { cmd_mouse(); return; }
    if (streq(line, "clear"))  { clrscr();    return; }
    if (streq(line, "help"))   { cmd_help();  return; }
    printf("unknown command: %s  (type 'help')\r\n", line);
}

// =====================================================
// Read-eval-print loop
// =====================================================

void Shell_Run(void)
{
    char line[LINE_MAX];
    int  pos;

    printf("\r\nType 'help' for commands.\r\n");

    for (;;)
    {
        printf("> ");
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
