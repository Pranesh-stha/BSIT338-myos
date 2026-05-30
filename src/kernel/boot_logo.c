#include "boot_logo.h"
#include "stdio.h"

// CP437 box-drawing in VGA text mode:
//   \xC9 = top-left  \xBB = top-right
//   \xC8 = bot-left  \xBC = bot-right
//   \xCD = horizontal  \xBA = vertical

static const char* g_LogoLines[] = {
    "  \xC9\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBB",
    "  \xBA                                                   \xBA",
    "  \xBA              ===  dhokaOS v0.1  ===              \xBA",
    "  \xBA          a hand-built x86 kernel                 \xBA",
    "  \xBA       bootloader -> kernel -> shell              \xBA",
    "  \xBA                                                   \xBA",
    "  \xC8\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBC",
    NULL
};

void BootLogo_Show(void)
{
    uint8_t saved = get_color();

    // Cyan frame
    set_color(VGA_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
    putc('\r'); putc('\n');
    puts(g_LogoLines[0]); putc('\r'); putc('\n');

    // Top side
    puts(g_LogoLines[1]); putc('\r'); putc('\n');

    // Title in yellow (inside the frame)
    set_color(VGA_COLOR(COLOR_YELLOW, COLOR_BLACK));
    puts(g_LogoLines[2]); putc('\r'); putc('\n');

    set_color(VGA_COLOR(COLOR_WHITE, COLOR_BLACK));
    puts(g_LogoLines[3]); putc('\r'); putc('\n');
    puts(g_LogoLines[4]); putc('\r'); putc('\n');

    // Bottom side
    set_color(VGA_COLOR(COLOR_LIGHT_CYAN, COLOR_BLACK));
    puts(g_LogoLines[5]); putc('\r'); putc('\n');
    puts(g_LogoLines[6]); putc('\r'); putc('\n');

    set_color(saved);
}
