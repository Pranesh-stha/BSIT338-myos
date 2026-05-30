#pragma once

#include "stdint.h"
#include <stdbool.h>

// =====================================================
// VGA text-mode color constants
// =====================================================
#define COLOR_BLACK         0x0
#define COLOR_BLUE          0x1
#define COLOR_GREEN         0x2
#define COLOR_CYAN          0x3
#define COLOR_RED           0x4
#define COLOR_MAGENTA       0x5
#define COLOR_BROWN         0x6
#define COLOR_LIGHT_GRAY    0x7
#define COLOR_DARK_GRAY     0x8
#define COLOR_LIGHT_BLUE    0x9
#define COLOR_LIGHT_GREEN   0xA
#define COLOR_LIGHT_CYAN    0xB
#define COLOR_LIGHT_RED     0xC
#define COLOR_LIGHT_MAGENTA 0xD
#define COLOR_YELLOW        0xE
#define COLOR_WHITE         0xF

#define VGA_COLOR(fg, bg)   (((bg) << 4) | (fg))
#define VGA_DEFAULT         0x07

void putc(char c);
void puts(const char* str);
void printf(const char* fmt, ...);
void print_buffer(const char* msg, const void* buffer, uint32_t count);

void clrscr(void);
void scrollback(int lines);

// Set color attribute for subsequent putc output.
void    set_color(uint8_t color);
uint8_t get_color(void);

// Direct VGA cell access (bypasses the cursor; for status bar,
// matrix rain, fractal, mouse cursor highlight, etc.)
void    vga_put_cell(int x, int y, char c, uint8_t color);
uint8_t vga_get_color(int x, int y);
void    vga_set_color(int x, int y, uint8_t color);
