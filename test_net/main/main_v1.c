
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
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

#define MAX_STR 32

#define STACK_SZ 2048 // receive task stack size
#define PRIORITY 4 // receive task priority
#define SEND_COUNT 20
#define SEND_DELAY 1000
#define GROUP_ID 1
#define GROUP_WAIT 4000 // ms

typedef struct {
	uint32_t i;
	char str[MAX_STR];
} event_t;

static const char *TAG = "test_net";
static TaskHandle_t rtask_h;
// static uint8_t broadcast_mac[NET_ALEN] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};


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

	// Open group registration.
	ret = net_group_open(GROUP_ID);
	if (ret) {
		ESP_LOGE(TAG, "net_group_open() fail:%ld", ret);
		return;
	}
	printf("Wait for devices to join group...\n");
	vTaskDelay(pdMS_TO_TICKS(GROUP_WAIT));
	ret = net_group_close();
	if (ret) {
		ESP_LOGE(TAG, "net_group_close() fail:%ld", ret);
		return;
	}
	ret = net_group_count();
	if (ret < 0) {
		ESP_LOGE(TAG, "net_group_count() fail:%ld", ret);
		return;
	} else {
		printf("Group count:%ld\n", ret);
	}

	// If there are devices in the group, then send.
	if (ret > 0) {
		for (uint32_t scnt = 0; scnt < SEND_COUNT; scnt++) {
			int32_t ret;
			event_t packet;
			packet.i = scnt;
			snprintf(packet.str, sizeof(packet.str), "msg");
			// ret = net_send(broadcast_mac, &packet, sizeof(packet), 0);
			ret = net_send(NULL, &packet, sizeof(packet), 0);
			if (ret != sizeof(packet)) {
				ESP_LOGE(TAG, "net_send() fail:%ld", ret);
			}
			vTaskDelay(pdMS_TO_TICKS(SEND_DELAY));
		}
	}

	// Test group clear.
	ret = net_group_clear();
	if (ret) {
		ESP_LOGE(TAG, "net_group_clear() fail:%ld", ret);
		return;
	}

	// Release resources.
	vTaskDelete(rtask_h); // Stop recv_task.
	rtask_h = NULL;
	ret = net_deinit();
	if (ret) {
		ESP_LOGE(TAG, "net_deinit() fail:%ld", ret);
	}
	printf("Terminate.\n");
}
