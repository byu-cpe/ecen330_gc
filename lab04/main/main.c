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

#include "hw.h"
#include "lcd.h"
#include "pin.h"
#include "joy.h"
#include "tone.h"

#define VOL_INC 20 // %
#define SAMPLE_RATE 24000 // Hz
#define VOLUME_DEFAULT (VOL_INC*3) // %
#define UP_PER 30 // Update period in ms
#define TIME_OUT 500 // ms

// Musical note frequencies
#define A3 220
#define A4 440
#define A5 880

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
#include "powerUp.h"

TimerHandle_t update_timer;
coord_t dcx, dcy;
uint32_t vol;
tone_t tone;

// Draw the cursor on the display
void cursor(coord_t x, coord_t y, color_t color)
{
	coord_t s2 = CUR_SZ >> 1; // size div 2
	lcd_drawHLine(x-s2, y,    CUR_SZ, color);
	lcd_drawVLine(x,    y-s2, CUR_SZ, color);
}

// Draw the status bar at the top of the display
void draw_status(void)
{
	char str[28];
	static char *ttab[] = {"sin", "squ", "tri", "saw"};
	sprintf(str, "x:%5ld y:%5ld v:%3lu t:%s", dcx, dcy, vol, ttab[tone]);
	lcd_drawString(0, 0, str, STB_CL);
}

// Update the position of the cursor and sound a tone if HW_BTN_A is pressed
void update(TimerHandle_t pxTimer)
{
	static coord_t lx = -1, ly = -1;
	static bool pressed = false;
	coord_t x, y;
	uint64_t btns;

	joy_get_displacement(&dcx, &dcy);
	btns = ~pin_get_in_reg() & HW_BTN_MASK;
	if (!pressed && btns) {
		pressed = true;
		if (!pin_get_level(HW_BTN_A))
			tone_start(tone, (dcy > 0) ? A4-(dcy*A3)/JOY_MAX_DISP : A4-(dcy*A4)/JOY_MAX_DISP);
		else if (!pin_get_level(HW_BTN_MENU))
			tone = (tone+1)%LAST_T;
		else if (!pin_get_level(HW_BTN_OPTION))
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
	pin_reset(HW_BTN_A);
	pin_input(HW_BTN_A, true);
	pin_reset(HW_BTN_B);
	pin_input(HW_BTN_B, true);
	pin_reset(HW_BTN_MENU);
	pin_input(HW_BTN_MENU, true);
	pin_reset(HW_BTN_OPTION);
	pin_input(HW_BTN_OPTION, true);
	pin_reset(HW_BTN_SELECT);
	pin_input(HW_BTN_SELECT, true);
	pin_reset(HW_BTN_START);
	pin_input(HW_BTN_START, true);

	lcd_init(); // Initialize LCD display and device handle
	lcd_fillScreen(SBG_CL); // Clear the screen
	lcd_setFontBackground(SBG_CL); // Set font background

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

	sound_start(powerUp, sizeof(powerUp), true); // Play power-up sound
	for (;;) vTaskDelay(pdMS_TO_TICKS(1000));
}
