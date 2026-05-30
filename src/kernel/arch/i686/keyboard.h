#ifndef ARCH_I686_KEYBOARD_H
#define ARCH_I686_KEYBOARD_H

#include <stdbool.h>

// Pseudo-ASCII codes emitted for the non-printable special keys.
// These slot into ASCII control-code space that the shell would never
// normally see from typed input.
#define KEY_UP      0x11
#define KEY_DOWN    0x12
#define KEY_LEFT    0x13
#define KEY_RIGHT   0x14

void i686_Keyboard_Initialize();
bool i686_Keyboard_HasKey();
char i686_Keyboard_GetChar();

#endif
