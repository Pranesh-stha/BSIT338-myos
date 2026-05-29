#pragma once

#include "stdint.h"
#include <stdbool.h>

void putc(char c);
void puts(const char* str);
void printf(const char* fmt, ...);
void print_buffer(const char* msg, const void* buffer, uint32_t count);

void clrscr(void);
void scrollback(int lines);
