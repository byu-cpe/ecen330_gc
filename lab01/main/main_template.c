#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "lcd.h"
#include "pac.h"

static const char *TAG = "lab01";

#define delayMS(ms) \
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
void drawCar(coord_t x, coord_t y)
{
	// TODO: Implement car procedurally with lcd geometric primitives.
}

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


void app_main(void)
{
	ESP_LOGI(TAG, "Start up");
	lcd_init();
	lcd_fillScreen(BACKGROUND_CLR);
	lcd_setFontSize(FONT_SIZE);
	lcd_drawString(0, 0, "Hello World! (lcd)", TITLE_CLR);
	printf("Hello World! (terminal)\n");
	delayMS(WAIT);

	// TODO: Exercise 1 - Draw car in one location.
	// * Fill screen with background color
	// * Draw string "Exercise 1" at top left of screen with title color
	// * Draw car at OBJ_X, OBJ_Y
	// * Wait 2 seconds

	// TODO: Exercise 2 - Draw moving car (Method 1), one pass across display.
	// Clear the entire display and redraw all objects each iteration.
	// Use a loop and increment x by OBJ_MOVE each iteration.
	// Start x off screen (negative coordinate).
	// Move loop:
	// * Fill screen with background color
	// * Draw string "Exercise 2" at top left of screen with title color
	// * Draw car at x, OBJ_Y
	// * Display the x position of the car at bottom left of screen
	//   with status color

	// TODO: Exercise 3 - Draw moving car (Method 2), one pass across display.
	// Move by erasing car at old position, then redrawing at new position.
	// Objects that don't change or move are drawn once.
	// Before loop:
	// * Fill screen once with background color
	// * Draw string "Exercise 3" at top left of screen with title color
	// Move loop:
	// * Erase car at old position by drawing a rectangle with background color
	// * Draw car at new position
	// * Erase status at bottom by drawing a rectangle with background color
	// * Display new position status of car at bottom left of screen
	// After running the above first, add a 20ms delay within the loop
	// at the end to see the effect.

	// TODO: Exercise 4 - Draw moving car (Method 3), one pass across display.
	// First, draw all objects into a cleared, off-screen frame buffer.
	// Then, transfer the entire frame buffer to the screen.
	// Before loop:
	// * Enable the frame buffer
	// Move loop:
	// * Fill screen (frame buffer) with background color
	// * Draw string "Exercise 4" at top left with title color
	// * Draw car at x, OBJ_Y
	// * Display position of the car at bottom left with status color
	// * Write the frame buffer to the LCD

	// TODO: Exercise 5 - Draw an animated Pac-Man moving across the display.
	// Use Pac-Man sprites instead of the car object.
	// Cycle through each sprite when moving the Pac-Man character.
	// Before loop:
	// * Enable the frame buffer
	// Nest the move loop inside a forever loop:
	// Move loop:
	// * Fill screen (frame buffer) with background color
	// * Draw string "Exercise 5" at top left with title color
	// * Draw Pac-Man at x, OBJ_Y with yellow color;
	//   Cycle through sprites to animate chomp
	// * Display position at bottom left with status color
	// * Write the frame buffer to the LCD
}
