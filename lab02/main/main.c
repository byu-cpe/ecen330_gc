#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h" // gpio_dump_io_configuration
#include "driver/rtc_io.h" // rtc_gpio_is_valid_gpio

#include "lcd.h"
#include "pin.h"
#include "pin_test.h"

static const char *TAG = "lab02";

// #define USE_GPIO 1

#define PIN_MAX  40
#define PIN_COLS 8
#define PIN_ROWS (PIN_MAX/PIN_COLS)
#define PIN_W    (LCD_W/PIN_COLS)
#define PIN_H    (PIN_W)
#define PIN_R    (PIN_W/4)

/*
SW1 IO32 BTN-A
SW2 IO33 BTN-B
SW3 IO13 BTN-MENU
SW4 IO00 BTN-OPTION
SW5 IO27 BTN-SELECT
SW6 IO39 BTN-START

A+ IO26
SD IO25

EX4 IO02
EX5 IO12
EX6 IO15
EX7 IO16
EX8 IO17
*/
bool in_pin[PIN_MAX] = {
	1, 0, 1, 0, 0, 0, 0, 0, // pin  0 -  7
	0, 0, 0, 0, 1, 1, 0, 1, // pin  8 - 15
	1, 1, 0, 0, 0, 0, 0, 0, // pin 16 - 23
	0, 0, 0, 1, 0, 0, 0, 0, // pin 24 - 31
	1, 1, 0, 0, 0, 0, 0, 1, // pin 32 - 39
};
bool out_pin[PIN_MAX] = {
	0, 0, 0, 0, 0, 0, 0, 0, // pin  0 -  7
	0, 0, 0, 0, 0, 0, 0, 0, // pin  8 - 15
	0, 0, 0, 0, 0, 0, 0, 0, // pin 16 - 23
	0, 1, 1, 0, 0, 0, 0, 0, // pin 24 - 31
	0, 0, 0, 0, 0, 0, 0, 0, // pin 32 - 39
};

bool pin_state[PIN_MAX];

// Draw a panel indicating the state of the I/O pins on the display
void draw_panel(TFT_t *dev)
{
	for (uint32_t i = 0; i < PIN_MAX; i++) {
		uint16_t x = (i%PIN_COLS)*PIN_W + PIN_W/2;
		uint16_t y = LCD_H - (PIN_ROWS-(i/PIN_COLS))*PIN_H + (PIN_H/2);

		if (in_pin[i] || out_pin[i]) {
#ifdef USE_GPIO
			pin_state[i] = gpio_get_level(i);
#else
			pin_state[i] = pin_get_level(i);
#endif // USE_GPIO
			lcdDrawCircle(dev, x, y, PIN_R, GREEN);
			lcdFillCircle(dev, x, y, PIN_R-1, pin_state[i] ? GREEN:BLACK);
		} else {
			lcdDrawCircle(dev, x, y, PIN_R, RED);
		}
	}
}

// Update the state of the I/O pins on the display
void update_panel(TFT_t *dev)
{
	static bool p26 = false;
	for (uint32_t i = 0; i < PIN_MAX; i++) {
		if (!(in_pin[i] || out_pin[i])) continue;
#ifdef USE_GPIO
		int32_t cps = gpio_get_level(i); // current pin state
#else
		int32_t cps = pin_get_level(i); // current pin state
#endif // USE_GPIO
		uint64_t crs = pin_get_in_reg(); // current in reg state
		if (cps != pin_state[i]) {
			uint16_t x = (i%PIN_COLS)*PIN_W + PIN_W/2;
			uint16_t y = LCD_H - (PIN_ROWS-(i/PIN_COLS))*PIN_H + (PIN_H/2);
			lcdFillCircle(dev, x, y, PIN_R-1, cps ? GREEN:BLACK);
			pin_state[i] = cps;
			ESP_LOGI(TAG, "IO%02u=%u", (unsigned)i, (unsigned)cps);
			// test if pin_get_in_reg() matches pin_get_level()
			if ((int32_t)((crs >> i) & 1U) != cps) {
				ESP_LOGE(TAG, "input reg: %010llX", crs);
			}
			// make a sound
			while (i == 39 && !pin_get_level(i)) pin_set_level(26, p26^=true);
			pin_set_level(26, p26=false);
		}
	}
}

// Main application
void app_main(void)
{
	TFT_t dev;
	uint64_t crs;

	ESP_LOGI(TAG, "Start up");
	lcdInit(&dev);
	lcdFillScreen(&dev, BLACK);

	for (pin_num_t i = 0; i < PIN_MAX; i++) {
		// only initialize specified pins
		if (!(in_pin[i] || out_pin[i])) continue;
#ifdef USE_GPIO
		gpio_reset_pin(i);
		gpio_set_pull_mode(i, out_pin[i] ? GPIO_FLOATING:GPIO_PULLUP_ONLY);
		// if output, set as input in addition so it can be read
		gpio_set_direction(i, out_pin[i] ? GPIO_MODE_INPUT_OUTPUT:GPIO_MODE_INPUT);
		if (out_pin[i]) gpio_set_level(i, 0);
#else
		pin_reset(i);
		pin_pullup(i, !out_pin[i]);
		// if output, set as input in addition so it can be read
		pin_input(i, true);
		if (out_pin[i]) {
			pin_output(i, true);
			pin_set_level(i, 0);
		}
#endif /* USE_GPIO */
		printf("rtc_gpio(%02hhd)  : %d\n", i, rtc_gpio_is_valid_gpio(i));
		printf("pin_reg(%02hhd)   : %lX\n", i, pin_get_pin_reg(i));
		printf("out_sel(%02hhd)   : %lX\n", i, pin_get_func_out_sel_cfg_reg(i));
		printf("io_mux_reg(%02hhd): %lX\n", i, pin_get_io_mux_reg(i));
		// gpio_dump_io_configuration(stdout, 1LLU << i);
	}

	// test sound output
	pin_set_level(25, 1);
	pin_set_level(26, 1);
	crs = pin_get_out_reg(); // current out reg state
	if ((int32_t)((crs >> 25) & 3U) != 3) {
		ESP_LOGE(TAG, "output reg: %010llX, Expect IO25:1 IO26:1", crs);
	}
	pin_set_level(26, 0);
	crs = pin_get_out_reg(); // current out reg state
	if ((int32_t)((crs >> 25) & 3U) != 1) {
		ESP_LOGE(TAG, "output reg: %010llX, Expect IO25:1 IO26:0", crs);
	}

	draw_panel(&dev);
	for (;;) {
		update_panel(&dev);
		vTaskDelay(1);
	}
}
