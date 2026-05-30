#include "speaker.h"
#include "io.h"
#include "pit.h"

#define PIT_BASE_FREQ   1193182
#define PIT_COMMAND     0x43
#define PIT_CH2_DATA    0x42
#define SPEAKER_PORT    0x61

// Configure PIT channel 2 to generate a square wave at the given frequency
// and open the gate so it drives the PC speaker.
void Speaker_PlayFreq(uint32_t hz)
{
    if (hz == 0) { Speaker_Off(); return; }

    uint16_t divisor = (uint16_t)(PIT_BASE_FREQ / hz);

    // Command byte:
    //   bits 6-7 = 10  (channel 2)
    //   bits 4-5 = 11  (lo-byte then hi-byte access)
    //   bits 1-3 = 011 (mode 3 - square wave)
    //   bit  0   = 0   (binary count)
    // => 0xB6
    i686_outb(PIT_COMMAND,  0xB6);
    i686_outb(PIT_CH2_DATA, (uint8_t)(divisor & 0xFF));
    i686_outb(PIT_CH2_DATA, (uint8_t)((divisor >> 8) & 0xFF));

    // Enable gate (bit 0) + speaker data (bit 1) on port 0x61.
    uint8_t cur = i686_inb(SPEAKER_PORT);
    if ((cur & 0x03) != 0x03)
        i686_outb(SPEAKER_PORT, cur | 0x03);
}

void Speaker_Off(void)
{
    uint8_t cur = i686_inb(SPEAKER_PORT);
    i686_outb(SPEAKER_PORT, cur & ~0x03);
}

void Speaker_Beep(uint32_t hz, uint32_t ms)
{
    Speaker_PlayFreq(hz);
    i686_PIT_Sleep(ms);
    Speaker_Off();
}

// =====================================================
// Canned melody: a short ascending arpeggio + flourish.
// Frequencies in Hz, durations in ms. hz=0 means rest.
// =====================================================
typedef struct { uint32_t hz; uint32_t ms; } Note;

static const Note g_Melody[] = {
    // C E G C arpeggio
    {523, 160}, {659, 160}, {784, 160}, {1047, 280},
    {  0,  60},
    // G E C cascade
    {784, 160}, {659, 160}, {523, 320},
    {  0,  80},
    // Final tonic
    {523, 100}, {1047, 380},
    {  0,   0},   // terminator
};

void Speaker_PlayMelody(void)
{
    for (int i = 0; g_Melody[i].ms != 0 || g_Melody[i].hz != 0; i++)
    {
        if (g_Melody[i].hz == 0)
        {
            Speaker_Off();
            i686_PIT_Sleep(g_Melody[i].ms);
        }
        else
        {
            Speaker_Beep(g_Melody[i].hz, g_Melody[i].ms);
            i686_PIT_Sleep(30);     // small gap between notes
        }
    }
    Speaker_Off();
}
