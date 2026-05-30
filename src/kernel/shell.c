#include "shell.h"
#include "stdio.h"
#include "arch/i686/keyboard.h"
#include "arch/i686/pit.h"
#include "pmm.h"

#define LINE_MAX 128

// Local strcmp — kernel doesn't have a string lib yet.
static int streq(const char* a, const char* b)
{
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == 0 && *b == 0;
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

static void cmd_help(void)
{
    printf("Commands:\r\n");
    printf("  time   - run a 5-second PIT timer test\r\n");
    printf("  mem    - print PMM stats\r\n");
    printf("  clear  - clear the screen\r\n");
    printf("  help   - this message\r\n");
}

static void exec(const char* line)
{
    if (line[0] == 0)            return;        // empty line
    if (streq(line, "time"))   { cmd_time();  return; }
    if (streq(line, "mem"))    { cmd_mem();   return; }
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
                    printf("%c", '\b');   // putc handles backspace
                }
                continue;
            }
            if (pos < LINE_MAX - 1)
            {
                line[pos++] = c;
                printf("%c", c);          // echo
            }
        }

        exec(line);
    }
}
