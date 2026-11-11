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

    //check to make sure malloc worked
    if (new_buffer == NULL) 
    {
        return 1;
    }

    //stop the sound and set things up
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
    //frees all the resouces 
    sound_stop();
    free(tone_waveform_buffer);
    tone_waveform_buffer = NULL;
    tone_sample_hz = 0;

    return sound_deinit();
}

// Start playing the specified tone.
// tone: one of the enumerated tone types.
// freq: frequency of the tone in Hz.
void tone_start(tone_t tone, uint32_t freq)
{
    //check the parameters to see if theyre in bounds
    if (tone_waveform_buffer == NULL || tone < SINE_T || tone >= LAST_T || freq < LOWEST_FREQ || freq > tone_sample_hz / 2U) 
    {
        return;
    }

    uint32_t samples = tone_sample_hz / freq;
    sound_stop();
    for (uint32_t i = 0; i < samples; i++) 
    {
        float phase = (float)i / (float)samples;
        float value = 0.0f;

        switch (tone) {
        case SINE_T:
            value = 127.5f +
                    127.5f * sinf(2.0f * 3.14159265f * phase);
            break;

        case SQUARE_T:
            value = (i < samples / 2U) ? 255.0f : 0.0f;
            break;

        case TRIANGLE_T:
            // Rise from 128 to 255.
            // Fall from 255 to 0.
            // Rise from 0 back toward 128.
            if (phase < 0.25f) 
            {    
                value = 128.0f + 508.0f * phase;
            } else if (phase < 0.75f) 
            {
                value = 255.0f - 510.0f * (phase - 0.25f);
            } else 
            {
                value = 512.0f * (phase - 0.75f);
            }
            break;

        case SAW_T:
            // Rise from 128 toward 255.
            // Jump to 0, then rise back toward 128.
            if (phase < 0.5f) 
            {
                value = 128.0f + 254.0f * phase;
            } else 
            {
                value = 256.0f * (phase - 0.5f);
            }
            break;

        default:
            return;
        }
        tone_waveform_buffer[i] = (uint8_t)(value + 0.5f);
    }

    sound_cyclic(tone_waveform_buffer, samples);

}

