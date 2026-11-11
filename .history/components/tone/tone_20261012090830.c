#include <stdlib.h>
#include <math.h>
#include <tone.h>

static const uint32_t MIN_SAMPLES_PER_PERIOD = 2U;
static const int32_t TONE_SUCCESS = 0;
static const int32_t TONE_ERROR = 1;

#define MIN_SAMPLES_PER_PERIOD 2U
#define TONE_SUCCESS 0
#define TONE_ERROR 1

#define SAMPLE_MIN 0.0f
#define SAMPLE_MAX 255.0f
#define SAMPLE_CENTER 128.0f
#define SINE_AMPLITUDE 127.5f
#define TWO_PI 6.2831853f
#define QUARTER_PERIOD 0.25f
#define HALF_PERIOD 0.5f
#define THREE_QUARTER_PERIOD 0.75f
#define SAMPLE_ROUNDING_OFFSET 0.5f

#define TRIANGLE_FIRST_RISE_SLOPE 508.0f
#define TRIANGLE_FALL_SLOPE 510.0f
#define TRIANGLE_LAST_RISE_SLOPE 512.0f
#define SAW_FIRST_RISE_SLOPE 254.0f
#define SAW_LAST_RISE_SLOPE 256.0f

static uint8_t *tone_waveform_buffer = NULL;
static uint32_t tone_sample_hz = 0;

// Initializes the tone driver. Must be called before using.
// May be called again to change sample rate.
// sample_hz: sample rate in Hz to playback tone.
// Return zero if successful, or non-zero otherwise.
int32_t tone_init(uint32_t sample_hz)
{
    // check for valid sample rate
    if (sample_hz < MIN_SAMPLES_PER_PERIOD * LOWEST_FREQ)
    {
    return TONE_ERROR;
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
        return TONE_ERROR;
    }

    //stop the sound and set things up
    sound_stop();
    free(tone_waveform_buffer);
    tone_waveform_buffer = new_buffer;
    tone_sample_hz = sample_hz;

    return TONE_SUCCESS;
}

// Frees resources used for tone generation (DAC, etc.).
// Return zero if successful, or non-zero otherwise.
int32_t tone_deinit(void)
{
    //frees all the resouces and deallocates memory
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
    if (tone_waveform_buffer == NULL || tone < SINE_T || tone >= LAST_T || freq < LOWEST_FREQ || freq > tone_sample_hz / MIN_SAMPLES_PER_PERIOD)
    {
        return;
    }

    uint32_t samples = tone_sample_hz / freq;
    sound_stop();

    //loops through and makes the wave forms sine, square, triangle and sawtooth
    for (uint32_t i = 0; i < samples; i++)
    {
        float phase = (float)i / (float)samples;
        float value = SAMPLE_MIN;

        //switches based on the tone
        switch (tone) {
        case SINE_T:
            value = SINE_AMPLITUDE +
                    SINE_AMPLITUDE * sinf(TWO_PI * phase);
            break;

        case SQUARE_T:
            if (i < samples / MIN_SAMPLES_PER_PERIOD)
            {
                value = SAMPLE_MAX;
            }
            else 
            {
                value = SAMPLE_MIN;
            }
            break;

        case TRIANGLE_T:
            // Rise from 128 to 255.
            // Fall from 255 to 0.
            // Rise from 0 back toward 128.
            if (phase < QUARTER_PERIOD)
            {
                value = SAMPLE_CENTER + TRIANGLE_FIRST_RISE_SLOPE * phase;
            } else if (phase < THREE_QUARTER_PERIOD)
            {
                value = SAMPLE_MAX - TRIANGLE_FALL_SLOPE * (phase - QUARTER_PERIOD);
            } else
            {
                value = TRIANGLE_LAST_RISE_SLOPE * (phase - THREE_QUARTER_PERIOD);
            }
            break;

        case SAW_T:
            // Rise from 128 toward 255.
            // Jump to 0, then rise back toward 128.
            if (phase < HALF_PERIOD)
            {
                value = SAMPLE_CENTER + SAW_FIRST_RISE_SLOPE * phase;
            } else
            {
                value = SAW_LAST_RISE_SLOPE * (phase - HALF_PERIOD);
            }
            break;

        default:
            return;
        }
        tone_waveform_buffer[i] = (uint8_t)(value + SAMPLE_ROUNDING_OFFSET);
    }

    sound_cyclic(tone_waveform_buffer, samples);

}

