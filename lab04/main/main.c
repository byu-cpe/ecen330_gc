// https://mixbutton.com/mixing-articles/music-note-to-frequency-chart/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_check.h"

#include "lcd.h"
#include "pin.h"
#include "joy.h"
#include "tone.h"

#define VOL_INC 20 // %
#define SAMPLE_RATE 48000 // Hz
#define VOLUME_DEFAULT (VOL_INC*3) // %
#define UP_PER 30 // Update period in ms
#define TIME_OUT 500 // ms

// Musical note frequencies
#define A3 220
#define A4 440
#define A5 880

// GPIO pins associated with buttons
#define BTN_A      32
#define BTN_B      33
#define BTN_MENU   13
#define BTN_OPTION  0
#define BTN_SELECT 27
#define BTN_START  39
#define BTN_MASK ( \
	1LLU << BTN_A | \
	1LLU << BTN_B | \
	1LLU << BTN_MENU | \
	1LLU << BTN_OPTION | \
	1LLU << BTN_SELECT | \
	1LLU << BTN_START \
	)

#define CUR_SZ 7 // Cursor size
#define CUR_CL WHITE // Cursor color
#define STB_CL GREEN // Status bar color
#define SBG_CL BLACK // Screen background color

#define OV 30 // Over scale by a margin
#define A2X(d) (((d)*(LCD_W/2+OV))/JOY_MAX_DISP+(LCD_W/2))
#define A2Y(d) (((d)*(LCD_H/2+OV))/JOY_MAX_DISP+(LCD_H/2))
#define CLIP(x,lo,hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))

static const char *TAG = "lab04";

// sound support
// #include "audio/bcFire48k.c"
// #include "audio/gameBoyStartup48k.c"
// #include "audio/gameOver48k.c"
// #include "audio/gunEmpty48k.c"
// #include "audio/ouch48k.c"
// #include "audio/pacmanDeath48k.c"
#include "audio/powerUp48k.c"
// #include "audio/screamAndDie48k.c"

TFT_t dev; // Declare device handle for the display
TimerHandle_t update_timer;
int32_t dcx, dcy;
uint32_t vol;
tone_t tone;

// Draw the cursor on the display
void cursor(int32_t x, int32_t y, uint16_t color)
{
	int32_t s2 = CUR_SZ >> 1; // size div 2
	lcdDrawHLine(&dev, x-s2, y,    CUR_SZ, color);
	lcdDrawVLine(&dev, x,    y-s2, CUR_SZ, color);
}

// Draw the status bar at the top of the display
void draw_status(void)
{
	char str[28];
	static char *ttab[] = {"sin", "squ", "tri", "saw"};
	sprintf(str, "x:%5ld y:%5ld v:%3lu t:%s", dcx, dcy, vol, ttab[tone]);
	lcdDrawString(&dev, 0, 0, str, STB_CL);
}

// Update the position of the cursor and sound a tone if BTN_A is pressed
void update(TimerHandle_t pxTimer)
{
	static int32_t lx = -1, ly = -1;
	static bool pressed = false;
	int32_t x, y;
	uint64_t btns;

	joy_get_displacement(&dcx, &dcy);
	btns = ~pin_get_in_reg() & BTN_MASK;
	if (!pressed && btns) {
		pressed = true;
		if (!pin_get_level(BTN_A))
			tone_start(tone, (dcy > 0) ? A4-(dcy*A3)/JOY_MAX_DISP : A4-(dcy*A4)/JOY_MAX_DISP);
		else if (!pin_get_level(BTN_MENU))
			tone = (tone+1)%LAST_T;
		else if (!pin_get_level(BTN_OPTION))
			tone_set_volume(vol = (vol <= MAX_VOL-VOL_INC) ? vol+VOL_INC : VOL_INC);
	} else if (pressed && !btns) {
		tone_stop();
		pressed = false;
	}
	draw_status();
	x = A2X(dcx); y = A2Y(dcy);
	x = CLIP(x, 0, LCD_W-1);
	y = CLIP(y, 0, LCD_H-1);
	if (x != lx || y != ly) {
		cursor( lx,  ly, SBG_CL);
		cursor(x, y, CUR_CL);
		lx = x; ly = y;
	}
}

// Main application
void app_main(void)
{
	ESP_LOGI(TAG, "Start up");

	// Configure I/O pins for buttons
	pin_reset(BTN_A);
	pin_input(BTN_A, true);
	pin_reset(BTN_B);
	pin_input(BTN_B, true);
	pin_reset(BTN_MENU);
	pin_input(BTN_MENU, true);
	pin_reset(BTN_OPTION);
	pin_input(BTN_OPTION, true);
	pin_reset(BTN_SELECT);
	pin_input(BTN_SELECT, true);
	pin_reset(BTN_START);
	pin_input(BTN_START, true);

	lcdInit(&dev); // Initialize LCD display and device handle
	lcdFillScreen(&dev, SBG_CL); // Clear the screen
	lcdSetFontBackground(&dev, SBG_CL); // Set font background

	tone_init(SAMPLE_RATE); // Initialize tone and sample rate
	tone_set_volume(vol = VOLUME_DEFAULT); // Set the volume
	tone = SINE_T;

	joy_init(); // Initialize joystick driver

	// Schedule the tick() function to run at the specified period.
	// The joy_get_displacement() function is not safe to call from an
	// ISR context. Therefore, the update() function must be called from
	// a software task context provided by the RTOS.
	update_timer = xTimerCreate(
		"update_timer",        // Text name for the timer.
		pdMS_TO_TICKS(UP_PER), // The timer period in ticks.
		pdTRUE,                // Auto-reload the timer when it expires.
		NULL,                  // No need for a timer ID.
		update                 // Tick function to call (callback) when timer expires.
	);
	if (update_timer == NULL) {
		ESP_LOGE(TAG, "Error creating timer");
		return;
	}
	if (xTimerStart(update_timer, pdMS_TO_TICKS(TIME_OUT)) != pdPASS) {
		ESP_LOGE(TAG, "Error starting timer");
		return;
	}

	sound_start(powerUp48k, sizeof(powerUp48k), true); // Play power-up sound
	for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
}
