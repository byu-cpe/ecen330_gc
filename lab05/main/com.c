#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "hw.h"
#include "lcd.h"
#include "net.h"
#include "com.h"

//creates a waiting screen before jumping straight into a game to try to connect with another board over wifi
int32_t com_init(void)
{
    int32_t ret = net_init();
    if (ret != 0) {
        return ret;
    }

    ret = net_group_open(1);
    if (ret != 0) {
        return ret;
    }

    lcd_drawString(0, 0, "Waiting to join group...", WHITE);
    
    vTaskDelay(pdMS_TO_TICKS(4000));

    return net_group_close();
}

//deinitializes all the values from the start
int32_t com_deinit(void)
{
    return net_deinit();
}

//sends a package of bytes that tell if/where a square has been marked to the other board
int32_t com_write(const void *buf, uint32_t size)
{
    return net_send(NULL, buf, size, 0);
}

//recieves a package of bytes that tell if or where a square has been marked on the other board
int32_t com_read(void *buf, uint32_t size)
{
    uint8_t src[NET_ALEN];

    return net_recv(src, buf, size, 0);
}