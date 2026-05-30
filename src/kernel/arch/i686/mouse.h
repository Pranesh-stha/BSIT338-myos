#ifndef ARCH_I686_MOUSE_H
#define ARCH_I686_MOUSE_H

#include "stdint.h"
#include <stdbool.h>

typedef struct {
    int32_t x;
    int32_t y;
    bool    leftButton;
    bool    rightButton;
    bool    middleButton;
} MouseState;

void       i686_Mouse_Initialize(void);
MouseState i686_Mouse_GetState(void);

#endif
