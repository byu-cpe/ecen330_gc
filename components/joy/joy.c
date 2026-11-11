#include "esp_adc/adc_oneshot.h"
#include "joy.h"
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "hw.h"

// static adc_unit_t unit[2];
// static adc_channel_t ADC_CHANNEL_6;
// static adc_channel_t ADC_CHANNEL_7;


// static adc_oneshot_unit_handle_t adc_handle;
static adc_oneshot_unit_handle_t adc1_handle;

static int32_t center_position_x; 
static int32_t center_position_y; 


static int_fast32_t rx;
static int_fast32_t ry;


//put comments
int32_t joy_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_7, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &config));

    
    int32_t xsum =0;
    int32_t ysum =0;
    for (int8_t i = 0; i < 10; i++)
    {
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &rx);
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &ry);
        xsum += rx;
        ysum += ry;

    }
    center_position_x = xsum/10;
    center_position_y = ysum/10;



    return 0;
}


int32_t joy_deinit(void)
{
    if(adc1_handle != NULL)
    {
        adc_oneshot_del_unit(adc1_handle);
    }
    return 0;
}

void joy_get_displacement(int32_t *dcx, int32_t *dcy)
{
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &rx);
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &ry);

    *dcx = rx - center_position_x;
    *dcy = ry - center_position_y;

}


