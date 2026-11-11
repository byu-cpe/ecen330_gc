#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "lcd.h"
#include "pac.h"

static const char *TAG = "lab01";

#define DELAY_MS(ms) \
	vTaskDelay(((ms)+(portTICK_PERIOD_MS-1))/portTICK_PERIOD_MS)

//----------------------------------------------------------------------------//
// Car Implementation - Begin
//----------------------------------------------------------------------------//

// Car constants
#define CAR_CLR rgb565(220,30,0)
#define WINDOW_CLR rgb565(180,210,238)
#define TIRE_CLR BLACK
#define HUB_CLR GRAY

// TODO: Finish car part constants

/**
 * @brief Draw a car at the specified location.
 * @param x      Top left corner X coordinate.
 * @param y      Top left corner Y coordinate.
 * @details Draw the car components relative to the anchor point (top, left).
 */


//----------------------------------------------------------------------------//
// Car Implementation - End
//----------------------------------------------------------------------------//

// Main display constants
#define BACKGROUND_CLR rgb565(0,60,90)
#define TITLE_CLR GREEN
#define STATUS_CLR WHITE
#define STR_BUF_LEN 12 // string buffer length
#define FONT_SIZE 2
#define FONT_W (LCD_CHAR_W*FONT_SIZE)
#define FONT_H (LCD_CHAR_H*FONT_SIZE)
#define STATUS_W (FONT_W*3)

#define WAIT 2000 // milliseconds
#define DELAY_EX3 20 // milliseconds

// Object position and movement
#define OBJ_X 100
#define OBJ_Y 100
#define OBJ_MOVE 3 // pixels

#define CAR_W 60
#define CAR_H 32

#define BODY_X0 0
#define BODY_Y0 12
#define BODY_X1 59
#define BODY_Y1 24

#define TOP_X0 0
#define TOP_Y0 0
#define TOP_X1 39
#define TOP_Y1 11

#define L_WINDOW_X0 3
#define L_WINDOW_Y0 1
#define L_WINDOW_X1 18
#define L_WINDOW_Y1 8

#define R_WINDOW_X0 21
#define R_WINDOW_Y0 1
#define R_WINDOW_X1 37
#define R_WINDOW_Y1 8

#define WINDOW_RADIUS 2

#define HOOD_X0 40
#define HOOD_Y0 9
#define HOOD_X1 40
#define HOOD_Y1 11
#define HOOD_X2 59
#define HOOD_Y2 11

#define L_TIRE_X0 11
#define L_TIRE_Y0 24


#define R_TIRE_X0 48
#define R_TIRE_Y0 24

#define TIRE_INNER_RADIUS 4
#define TIRE_OUTER_RADIUS 7


#define CAR_ERASE_X0 0
#define CAR_ERASE_Y0 0
#define CAR_ERASE_X1 60
#define CAR_ERASE_Y1 32




//draws car
void drawCar(coord_t x, coord_t y)
{
	lcd_fillRect2(x + BODY_X0, y + BODY_Y0, x + BODY_X1, y + BODY_Y1,CAR_CLR);
	lcd_fillRect2(x+ TOP_X0,y+ TOP_Y0,x+ TOP_X1,y+ TOP_Y1, CAR_CLR);
	lcd_fillRoundRect2(x+ L_WINDOW_X0,y+ L_WINDOW_Y0,x+ L_WINDOW_X1,y+ L_WINDOW_Y1, WINDOW_RADIUS, WINDOW_CLR);
	lcd_fillRoundRect2(x+ R_WINDOW_X0,y+ R_WINDOW_Y0,x+ R_WINDOW_X1,y+ R_WINDOW_Y1, WINDOW_RADIUS, WINDOW_CLR);
	lcd_fillTriangle(x+ HOOD_X0,y+ HOOD_Y0,x+ HOOD_X1,y+ HOOD_Y1,x+ HOOD_X2,y+ HOOD_Y2, CAR_CLR);
	lcd_fillCircle(x+ L_TIRE_X0,y+ L_TIRE_Y0, TIRE_OUTER_RADIUS, TIRE_CLR);
	lcd_fillCircle(x+ L_TIRE_X0,y+ L_TIRE_Y0, TIRE_INNER_RADIUS, HUB_CLR);
	lcd_fillCircle(x+ R_TIRE_X0, y+ R_TIRE_Y0, TIRE_OUTER_RADIUS, TIRE_CLR);
	lcd_fillCircle(x+ R_TIRE_X0, y+ R_TIRE_Y0, TIRE_INNER_RADIUS, HUB_CLR);

}

//erases car
void eraseCar(coord_t x, coord_t y)
{
	lcd_fillRect2(x + CAR_ERASE_X0, y + CAR_ERASE_Y0, x + CAR_ERASE_X1, y + CAR_ERASE_Y1,BACKGROUND_CLR);
	
}


// Application main
void app_main(void)
{
	ESP_LOGI(TAG, "Start up");
	lcd_init();
	lcd_fillScreen(BACKGROUND_CLR);
	lcd_setFontSize(FONT_SIZE);
	lcd_drawString(0,0 , "Hello World", TITLE_CLR);
	DELAY_MS(WAIT);


	// TODO: Exercise 1 - Draw car in one location.
	lcd_fillScreen(BACKGROUND_CLR);
	lcd_drawString(0,0 , "Exercise 1", TITLE_CLR);
	drawCar(OBJ_X, OBJ_Y);
	DELAY_MS(WAIT);

	// TODO: Exercise 2 - Draw moving car (Method 1), one pass across display.
	// Clear the entire display and redraw all objects each iteration.
	// Use a loop and increment x by OBJ_MOVE each iteration.
	// Start x off screen (negative coordinate).
	for (coord_t x = -CAR_W; x <= LCD_W; x += OBJ_MOVE)
	{
		lcd_fillScreen(BACKGROUND_CLR);
		lcd_drawString(0,0 , "Exercise 2", TITLE_CLR);
		drawCar(x, OBJ_Y);
		char str[STR_BUF_LEN];
		sprintf(str, "%3ld", x);
		lcd_drawString(0, LCD_H - FONT_H, str, STATUS_CLR);
	}


	// TODO: Exercise 3 - Draw moving car (Method 2), one pass across display.
	// Move by erasing car at old position, then redrawing at new position.
	// Objects that don't change or move are drawn once.
	lcd_fillScreen(BACKGROUND_CLR);
	lcd_drawString(0,0 , "Exercise 3", TITLE_CLR);
	for (coord_t x = -CAR_W; x <= LCD_W; x += OBJ_MOVE)
	{
		eraseCar(x-OBJ_MOVE,OBJ_Y);
		drawCar(x, OBJ_Y);
		char str[STR_BUF_LEN];
		sprintf(str, "%3ld", x);
		lcd_fillRect(0,LCD_H-FONT_H, LCD_W, FONT_H,BACKGROUND_CLR);
		lcd_drawString(0, LCD_H - FONT_H, str, STATUS_CLR);

	}

	
	// TODO: Exercise 4 - Draw moving car (Method 3), one pass across display.
	// First, draw all objects into a cleared, off-screen frame buffer.
	// Then, transfer the entire frame buffer to the screen.
	lcd_frameEnable();

	for (coord_t x = -CAR_W; x <= LCD_W; x += OBJ_MOVE)
	{
		lcd_fillScreen(BACKGROUND_CLR);
		lcd_drawString(0,0 , "Exercise 4", TITLE_CLR);
		drawCar(x, OBJ_Y);
		char str[STR_BUF_LEN];
		sprintf(str, "%3ld", x);
		lcd_drawString(0, LCD_H - FONT_H, str, STATUS_CLR);
		lcd_writeFrame();
	}
	// TODO: Exercise 5 - Draw an animated Pac-Man moving across the display.
	// Use Pac-Man sprites instead of the car object.
	// Cycle through each sprite when moving the Pac-Man character.

	//loops through cycles of pacman crossing the screen
	for (;;)
	{
	uint16_t i = 0;
	const uint8_t pidx[] = {0, 1, 2, 1};
	//draws pac man crossing the screen
	for (coord_t x = -CAR_W; x <= LCD_W; x += OBJ_MOVE)
		{
		lcd_fillScreen(BACKGROUND_CLR);
		lcd_drawString(0,0 , "Exercise 5", TITLE_CLR);
		lcd_drawBitmap(x, OBJ_Y, pac[pidx[i++ % sizeof(pidx)]],PAC_W,PAC_H , YELLOW);
		char str[STR_BUF_LEN];
		sprintf(str, "%3ld", x);
		lcd_drawString(0, LCD_H - FONT_H, str, STATUS_CLR);
		lcd_writeFrame();
		}
	}

}
