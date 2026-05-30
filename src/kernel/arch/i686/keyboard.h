#ifndef ARCH_I686_KEYBOARD_H
#define ARCH_I686_KEYBOARD_H

#include <stdbool.h>

void i686_Keyboard_Initialize();
bool i686_Keyboard_HasKey();
char i686_Keyboard_GetChar();

#endif
