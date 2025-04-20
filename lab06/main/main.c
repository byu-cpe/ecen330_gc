#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "hw.h"
#include "lcd.h"
#include "cursor.h"
#include "sound.h"
#include "pin.h"
#include "game.h"
#include "config.h"

// sound support
#include "missileLaunch.h"

// The update period as an integer in ms
#define PER_MS ((uint32_t)(CONFIG_GAME_TIMER_PERIOD*1000))
#define TIME_OUT 500 // ms

#define CURSOR_SZ 7 // Cursor size (width & height) in pixels

static const char *TAG = "lab06";

TimerHandle_t update_timer; // Declare timer handle for update callback

volatile bool interrupt_flag;

uint32_t isr_triggered_count;
uint32_t isr_handled_count;

// Interrupt handler for game - use flag method
void update() {
	interrupt_flag = true;
	isr_triggered_count++;
}

// Draw the cursor on the screen
void cursor(coord_t x, coord_t y, color_t color)
{
	coord_t s2 = CURSOR_SZ >> 1; // size div 2
	lcd_drawHLine(x-s2, y,    CURSOR_SZ, color);
	lcd_drawVLine(x,    y-s2, CURSOR_SZ, color);
}

// Main application
void app_main(void)
{
	// ISR flag and counts
	interrupt_flag = false;
	isr_triggered_count = 0;
	isr_handled_count = 0;

	// Initialization
	lcd_init();
	lcd_frameEnable();
	lcd_fillScreen(CONFIG_COLOR_BACKGROUND);
	cursor_init(PER_MS);
	sound_init(MISSILELAUNCH_SAMPLE_RATE);
	game_init();

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

	// Initialize update timer
	update_timer = xTimerCreate(
		"update_timer",        // Text name for the timer.
		pdMS_TO_TICKS(PER_MS), // The timer period in ticks.
		pdTRUE,                // Auto-reload the timer when it expires.
		NULL,                  // No need for a timer ID.
		update                 // Function called when timer expires.
	);
	if (update_timer == NULL) {
		ESP_LOGE(TAG, "Error creating update timer");
		return;
	}
	if (xTimerStart(update_timer, pdMS_TO_TICKS(TIME_OUT)) != pdPASS) {
		ESP_LOGE(TAG, "Error starting update timer");
		return;
	}

	// Main game loop
	uint64_t t1, t2, tmax = 0; // For hardware timer values
	coord_t x, y; // For cursor position
	while (pin_get_level(HW_BTN_MENU)) // while MENU button not pressed
	{
		while (!interrupt_flag) ;
		t1 = esp_timer_get_time();
		interrupt_flag = false;
		isr_handled_count++;

#ifndef CONFIG_ERASE
		lcd_fillScreen(CONFIG_COLOR_BACKGROUND);
#endif // CONFIG_ERASE
		game_tick();
		cursor_tick();
		cursor_get_pos(&x, &y);
#ifdef CONFIG_ERASE
		static coord_t lx = -1, ly = -1;
		if (x != lx || y != ly) {
			cursor(lx,  ly, CONFIG_COLOR_BACKGROUND);
			lx = x; ly = y;
		}
#endif // CONFIG_ERASE
		cursor(x, y, CONFIG_COLOR_CURSOR);
		lcd_writeFrame();
		t2 = esp_timer_get_time() - t1;
		if (t2 > tmax) tmax = t2;
	}
	printf("Handled %lu of %lu interrupts\n", isr_handled_count, isr_triggered_count);
	printf("WCET us:%llu\n", tmax);
	sound_deinit();
}
