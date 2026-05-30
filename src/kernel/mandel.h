#ifndef MANDEL_H
#define MANDEL_H

// Renders the Mandelbrot set in ASCII over the text area, using 16.16
// fixed-point math (no FPU required). Blocks until done. Returns to
// caller; caller can wait for a keypress before clearing.
void Mandel_Render(void);

#endif
