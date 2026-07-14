#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "hw.h"
#include "pin.h"
#include "lcd.h"
#include "nav.h"
#if MILESTONE == 2
#include "com.h"
#endif // MILESTONE
#include "graphics.h"
#include "game.h"
#include "config.h"

static const char *TAG = "lab05";

// The update period as an integer in ms
#define PER_MS ((uint32_t)(CONFIG_GAME_TIMER_PERIOD*1000))
#define TIME_OUT 500 // ms

#define CHK_RET(x) ({                                           \
        int32_t ret_val = (x);                                  \
        if (ret_val != 0) {                                     \
            ESP_LOGE(TAG, "FAIL: return %ld, %s", ret_val, #x); \
        }                                                       \
        ret_val;                                                \
    })

TimerHandle_t update_timer; // Declare timer handle for update callback

volatile bool interrupt_flag;

uint32_t isr_triggered_count;
uint32_t isr_handled_count;

// Interrupt handler for game - use flag method
void update(TimerHandle_t xTimer) {
	interrupt_flag = true;
	isr_triggered_count++;
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
	lcd_fillScreen(CONFIG_BACK_CLR);
	CHK_RET(nav_init(PER_MS));
#if MILESTONE == 2
	com_init();
#endif // MILESTONE
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
	int8_t r, c; // For navigator location
	while (pin_get_level(HW_BTN_MENU)) // while MENU button not pressed
	{
		while (!interrupt_flag) ;
		t1 = esp_timer_get_time();
		interrupt_flag = false;
		isr_handled_count++;

		game_tick();
		nav_tick();
		nav_get_loc(&r, &c);
		static int8_t lr = -1, lc = -1;
		if (r != lr || c != lc) {
			graphics_drawHighlight(lr,  lc, CONFIG_BACK_CLR);
			lr = r; lc = c;
		}
		graphics_drawHighlight(r, c, CONFIG_HIGH_CLR);
		t2 = esp_timer_get_time() - t1;
		if (t2 > tmax) tmax = t2;
	}
	printf("Handled %lu of %lu interrupts\n", isr_handled_count, isr_triggered_count);
	printf("WCET us:%llu\n", tmax);
}
