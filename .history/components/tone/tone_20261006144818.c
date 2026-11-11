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
    // check for valid sample rate
    if (sample_hz < 2U * LOWEST_FREQ) 
    {
    return 1;
    }

    //initilize sound_init()
    sound_init(sample_hz);

    // Round up to hold a complete period at the lowest frequency.
    uint32_t buffer_size = sample_hz / LOWEST_FREQ;
    if (sample_hz % LOWEST_FREQ != 0U) {
        buffer_size++;
    }

    uint8_t *new_buffer = malloc(buffer_size * sizeof(*new_buffer));

    sound_stop();
    free(tone_waveform_buffer);
    tone_waveform_buffer = new_buffer;
    tone_sample_hz = sample_hz;

    return 0;
}

// Frees resources used for tone generation (DAC, etc.).
// Return zero if successful, or non-zero otherwise.
int32_t tone_deinit(void)
{
    free(tone_waveform_buffer);
    tone_waveform_buffer = NULL;
    return sound_deinit;
}

// Start playing the specified tone.
// tone: one of the enumerated tone types.
// freq: frequency of the tone in Hz.
void tone_start(tone_t tone, uint32_t freq)
{
    uint32_t samples = tone_sample_hz / freq;
    sound_stop();
}

