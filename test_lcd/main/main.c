#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "test_lcd.h"

static const char *TAG = "test_lcd";

void app_main(void)
{
	ESP_LOGI(TAG, "Start up");

	test_lcd_all(NULL);
}
