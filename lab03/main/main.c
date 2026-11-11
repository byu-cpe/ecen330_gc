#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gptimer.h"
#include "hw.h"
#include "lcd.h"
#include "pin.h"
#include "watch.h"
#include "esp_timer.h"

static const char *TAG = "lab03";
bool RUNNING_FLAG;
int64_t timer_ticks;
int64_t start, finish;
volatile int64_t isr_max =0;
volatile int32_t isr_cnt = 0;





static bool call_back(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{   
    start = esp_timer_get_time();
    //check if button A is pressed
    if(!pin_get_level(HW_BTN_A))
    {
        RUNNING_FLAG = true;
    }
    //check if buttom B is pressed
    if(!pin_get_level(HW_BTN_B))
    {
        RUNNING_FLAG = false;
    }
    //check if start is pressed
    if(!pin_get_level(HW_BTN_START))
    {
        timer_ticks = 0;
        RUNNING_FLAG = false;
    }
    //incremement timer
    if(RUNNING_FLAG)
    {
        timer_ticks++;
    }
    finish = esp_timer_get_time();
    //sets isr max outside of the function so the print statement isn't running here so many times
    if ((finish-start) > isr_max)
    {
        isr_max = (finish-start);
    }
    isr_cnt++;
    return false;
}




// Main application
void app_main(void)
{
	ESP_LOGI(TAG, "Starting");

    //configures I/O pins for A B and START
    start = esp_timer_get_time();
    pin_reset(HW_BTN_A);
    pin_input(HW_BTN_A, true);
    pin_reset(HW_BTN_B);
    pin_input(HW_BTN_B, true);
    pin_reset(HW_BTN_START);
    pin_input(HW_BTN_START, true);
    finish = esp_timer_get_time();
    printf("Configure I/O pins time:%lld microseconds\n", finish-start);
    start = esp_timer_get_time();

    
    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT, // Select the default clock source
        .direction = GPTIMER_COUNT_UP,      // Counting direction is up
        .resolution_hz = 1 * 1000 * 1000,   // Resolution is 1 MHz, i.e., 1 tick equals 1 microsecond
    };
    // Create a timer instance
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

    //call_back();

    gptimer_alarm_config_t alarm_config = {
    .reload_count = 0,      // When the alarm event occurs, the timer will automatically reload to 0
    .alarm_count = 10000, // Set the actual alarm period, since the resolution is 1us, 1000000 represents 1s
    .flags.auto_reload_on_alarm = true, // Enable auto-reload function
    };
    // Set the timer's alarm action
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = call_back, // Call the user callback function when the alarm event occurs
    };
    // Register timer event callback functions, allowing user context to be carried
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
    // Enable the timer
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    // Start the timer
    ESP_ERROR_CHECK(gptimer_start(gptimer));

    finish = esp_timer_get_time();
    printf("Configure stopwatch timer time:%lld microseconds\n", finish-start);
    //start next timer block
    start = esp_timer_get_time();
    ESP_LOGI(TAG, "Stopwatch update");
    finish = esp_timer_get_time();
    printf("ESP_LOGI time:%lld microseconds\n", finish-start);

    lcd_init(); // Initialize LCD display
    watch_init(); // Initialize stopwatch face
    for (;;) { // forever update loop
        watch_update(timer_ticks);
        //checks to make sure the timer hasn't been going to long and then restarts it after 500 ticks
        if (isr_cnt > 500)
        {
            printf("ISR_max:%lld microseconds\n", isr_max);
            isr_max = 0;
            isr_cnt = 0;

        }
    }
}