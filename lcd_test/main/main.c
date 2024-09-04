#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "lcd_test.h"

static const char *TAG = "lcd_test";

void app_main(void)
{
	ESP_LOGI(TAG, "Start up");

	lcd_test_all(NULL);
}
