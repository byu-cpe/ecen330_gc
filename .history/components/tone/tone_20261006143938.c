#include <stdlib.h>
#include <math.h>
#include <tone.h>


static uint8_t *tone_waveform_buffer = NULL;
static uint32_t tone_sample_hz = 0;

// Initializes the tone driver. Must be called before using.
// May be called again to change sample rate.
// sample_hz: sample rate in Hz to playback tone.
// Return zero if successful, or non-zero otherwise.
int32_t tone_init(uint32_t sample_hz)
{
    //
    if (sample_hz < 2U * LOWEST_FREQ) 
    {
    return 1;
    }

    sound_init(sample_hz);

    return 0;
}

// Frees resources used for tone generation (DAC, etc.).
// Return zero if successful, or non-zero otherwise.
int32_t tone_deinit(void)
{
    return 0;
}

// Start playing the specified tone.
// tone: one of the enumerated tone types.
// freq: frequency of the tone in Hz.
void tone_start(tone_t tone, uint32_t freq)
{

}

