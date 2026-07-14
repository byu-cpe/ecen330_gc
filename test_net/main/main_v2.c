
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "driver/gpio.h" // gpio_*
#include "esp_mac.h" // MACSTR, MAC2STR
#include "esp_log.h"

#include "hw.h"
#include "net.h"
#include "lcd.h"
#include "lcd_printf.h"

#ifdef LCD_PRINTF_H_
#define printf(...) lcd_printf(__VA_ARGS__)
#undef ESP_LOGE
#define ESP_LOGE(tag, ...) lcd_printf(__VA_ARGS__); lcd_printf("\n")
#endif

#define TPERIOD 40 // timer period in ms
#define TO_PERIOD 500 // time out period in ms

#define MAX_STR 32

#define STACK_SZ 2048 // receive task stack size
#define PRIORITY 4 // receive task priority

typedef struct {
	uint32_t i;
	char str[MAX_STR];
} event_t;

static const char *TAG = "test_net";
static TaskHandle_t rtask_h; // Receive task 
static TimerHandle_t timer_h; // Timer for application tick function

// static uint8_t broadcast_mac[NET_ALEN] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// Application tick function
static void app_tick(TimerHandle_t pt)
{
	static bool gopen = false; // Group registration open
	static int32_t gcnt = 0; // Group count
	static uint32_t group_id = 0; // Group ID
	static uint32_t seq = 0; // Sequence number (trigger event)
	int32_t ret;
	// Read buttons
	static bool pressed = false;
	uint64_t btns = 0;
	btns |= (uint64_t)!gpio_get_level(HW_BTN_START) << HW_BTN_START;
	btns |= (uint64_t)!gpio_get_level(HW_BTN_A) << HW_BTN_A;
	btns |= (uint64_t)!gpio_get_level(HW_BTN_B) << HW_BTN_B;
	if (!pressed && btns) {
		pressed = true;
		// Determine which button
		if (btns & (1ULL << HW_BTN_START)) {
			event_t packet;
			packet.i = seq++;
			snprintf(packet.str, sizeof(packet.str), "grp%lu", group_id);
			ret = net_send(NULL, &packet, sizeof(packet), 0);
			if (ret != sizeof(packet)) {
				ESP_LOGE(TAG, "net_send() fail:%ld", ret);
			}
		} else {
			if      (btns & (1ULL << HW_BTN_A)) group_id = 1;
			else if (btns & (1ULL << HW_BTN_B)) group_id = 2;
			else group_id = 0;
			if (group_id) {
				gopen = true;
				// Group clear.
				ret = net_group_clear();
				if (ret) {
					ESP_LOGE(TAG, "net_group_clear() fail:%ld", ret);
				}
				// Open group registration.
				ret = net_group_open(group_id);
				if (ret) {
					ESP_LOGE(TAG, "net_group_open() fail:%ld", ret);
				}
				printf("Wait for devices to join group...\n");
				gcnt = -1;
			}
		}
	} else if (pressed && !btns) {
		pressed = false;
		if (gopen) {
			gopen = false;
			// Close group registration.
			ret = net_group_close();
			if (ret) {
				ESP_LOGE(TAG, "net_group_close() fail:%ld", ret);
			}
			// Get group count.
			ret = net_group_count();
			if (ret < 0) {
				ESP_LOGE(TAG, "net_group_count() fail:%ld", ret);
			} else {
				gcnt = ret;
				printf("Group count:%ld final\n", ret);
			}
		}
	} else if (gopen) {
			// Get group count.
			ret = net_group_count();
			if (ret < 0) {
				ESP_LOGE(TAG, "net_group_count() fail:%ld", ret);
			} else if (ret != gcnt) {
				gcnt = ret;
				printf("Group count:%ld\r", ret);
			}
	}
}

// Receive packets and display contents.
static void recv_task(void *pvParameter)
{
	uint32_t rcnt = 0; // receive count

	for (;;) {
		int32_t ret;
		uint8_t src[NET_ALEN];
		event_t packet;
		memset(&packet, 0, sizeof(packet));
		ret = net_recv(src, &packet, sizeof(packet), NET_MAX_WAIT);
		if (ret < 0 || ret != sizeof(packet)) {
			ESP_LOGE(TAG, "net_recv() fail:%ld", ret);
			continue;
		}
		printf("RX %ld \"%.32s\" from "MACSTR"\n",
			packet.i, packet.str, MAC2STR(src));
		rcnt++;
	}
}


// Main application
void app_main(void)
{
	int32_t ret;

	ESP_LOGI(TAG, "app_main");
	lcd_init(); // Clears display

	gpio_reset_pin(HW_BTN_START);
	gpio_pullup_en(HW_BTN_START);
	gpio_set_direction(HW_BTN_START, GPIO_MODE_INPUT);
	gpio_reset_pin(HW_BTN_A);
	gpio_pullup_en(HW_BTN_A);
	gpio_set_direction(HW_BTN_A, GPIO_MODE_INPUT);
	gpio_reset_pin(HW_BTN_B);
	gpio_pullup_en(HW_BTN_B);
	gpio_set_direction(HW_BTN_B, GPIO_MODE_INPUT);

	// Initialize network.
	ret = net_init();
	// esp_log_set_vprintf(lcd_vprintf);
	if (ret) {
		ESP_LOGE(TAG, "net_init() fail:%ld", ret);
		return;
	}
	// Create the receive task.
	if (xTaskCreate(recv_task, "recv_task", STACK_SZ, NULL, PRIORITY, &rtask_h) != pdPASS) {
		ESP_LOGE(TAG, "xTaskCreate() fail");
		return;
	}
	// Initialize timer
	timer_h = xTimerCreate(
		"tick_timer",           // Text name for the timer.
		pdMS_TO_TICKS(TPERIOD), // The timer period in ticks.
		pdTRUE,                 // Auto-reload the timer when it expires.
		NULL,                   // No need for a timer ID.
		app_tick                // Function called when timer expires.
	);
	if (timer_h == NULL) {
		ESP_LOGE(TAG, "Timer create fail");
		return;
	}
	if (xTimerStart(timer_h,
		pdMS_TO_TICKS(TO_PERIOD)) != pdPASS) {
		ESP_LOGE(TAG, "Timer start fail");
		return;
	}
	// Instructions
	printf("Join a group by holding a button.\n");
	printf("BTN_A:1 BTN_B:2\n");
	printf("Press Start to send message to group.\n");

#if 0
	// Release resources.
	vTaskDelete(rtask_h); // Stop recv_task.
	rtask_h = NULL;
	ret = net_deinit();
	if (ret) {
		ESP_LOGE(TAG, "net_deinit() fail:%ld", ret);
	}
	printf("Terminate.\n");
#endif
}
