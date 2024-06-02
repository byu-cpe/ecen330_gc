#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "config.h"
#include "lcd.h"
#include "cursor.h"
#include "pin.h"
#include "btn.h"
#include "gameControl.h"

// The update period as an integer in ms
#define PER_MS ((uint32_t)(CONFIG_GAME_TIMER_PERIOD*1000))
#define TIME_OUT 500 // ms

#define CURSOR_SZ 7 // Cursor size (width & height) in pixels

static const char *TAG = "lab05";

TFT_t dev; // Declare device handle for the display
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
void cursor(int32_t x, int32_t y, uint16_t color)
{
	int32_t s2 = CURSOR_SZ >> 1; // size div 2
	lcdDrawHLine(&dev, x-s2, y,    CURSOR_SZ, color);
	lcdDrawVLine(&dev, x,    y-s2, CURSOR_SZ, color);
}

// Test application
void app_main(void)
{
	// ISR flag and counts
	interrupt_flag = false;
	isr_triggered_count = 0;
	isr_handled_count = 0;

	// Initialization
	lcdInit(&dev);
	lcdFrameEnable(&dev);
	lcdFillScreen(&dev, CONFIG_COLOR_BACKGROUND);
	cursor_init(PER_MS);
	gameControl_init();

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
	int32_t x, y; // For cursor position
	while (pin_get_level(BTN_MENU)) // while MENU button not pressed
	{
		while (!interrupt_flag) ;
		t1 = esp_timer_get_time();
		interrupt_flag = false;
		isr_handled_count++;

#ifndef CONFIG_ERASE
		lcdFillScreen(&dev, CONFIG_COLOR_BACKGROUND);
#endif // CONFIG_ERASE
		gameControl_tick();
		cursor_tick();
		cursor_get_pos(&x, &y);
#ifdef CONFIG_ERASE
		static int32_t lx = -1, ly = -1;
		if (x != lx || y != ly) {
			cursor(lx,  ly, CONFIG_COLOR_BACKGROUND);
			lx = x; ly = y;
		}
#endif // CONFIG_ERASE
		cursor(x, y, CONFIG_COLOR_CURSOR);
		lcdWriteFrame(&dev);
		t2 = esp_timer_get_time() - t1;
		if (t2 > tmax) tmax = t2;
	}
	printf("Handled %lu of %lu interrupts\n", isr_handled_count, isr_triggered_count);
	printf("WCET us:%llu\n", tmax);
}
