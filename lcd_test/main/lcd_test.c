#include <stdio.h>
#include <stdint.h>
#include <stdlib.h> // rand, srand
#include <string.h> // strcpy, strlen
#include <time.h> // time (used with srand)

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h" // esp_timer_get_time

#include "lcd.h"
#include "crosshair.h"
#include "peppers.h"

// Time support
#define TICKS_SEC 1000000LL
#define PRINT_TIME(ticks) \
	ESP_LOGI(__FUNCTION__, "elapsed time[us]:%"PRIi64,(ticks))

#define WAIT vTaskDelay(200)

#define RAND_COLOR() ((color_t)rand())

static const coord_t width = LCD_W;
static const coord_t height = LCD_H;


int64_t lcd_test_colorBar(void) {
	int64_t startTick, endTick, diffTick;

	coord_t x1, x2;
	x1 = width/3;
	x2 = width*2/3;

	startTick = esp_timer_get_time();
	lcd_fillRect( 0, 0,    x1   , height, RED);
	lcd_fillRect(x1, 0,    x2-x1, height, GREEN);
	lcd_fillRect(x2, 0, width-x2, height, BLUE);
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_colorBand(void) {
	int64_t startTick, endTick, diffTick;

	color_t color = RED;
	coord_t delta = height/16;
	coord_t ypos = 0;
	lcd_fillScreen(BLACK);

	startTick = esp_timer_get_time();
	for (int32_t i = 0; i < 16; i++) {
		lcd_fillRect(0, ypos, width, delta, color);
		color = color >> 1;
		ypos += delta;
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

//----------------------------------------------------------------------------//
// Draw (outline) and fill primitives
//----------------------------------------------------------------------------//

int64_t lcd_test_fillScreen(void) {
	int64_t startTick, endTick, diffTick;

	color_t ctab[] = {RED,GREEN,BLUE,BLACK,GRAY,YELLOW,CYAN,MAGENTA};

	startTick = esp_timer_get_time();
	for (int32_t i = 0; i < 16; i++) {
		lcd_fillScreen(ctab[i%8]);
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

// lcd_test_drawPixel

// lcd_test_drawHPixels

int64_t lcd_test_drawHVLine(void) {
	int64_t startTick, endTick, diffTick;

	color_t color = RED;
	lcd_fillScreen(BLACK);

	startTick = esp_timer_get_time();
	for (coord_t ypos=0 ; ypos < height; ypos += 10) {
		lcd_drawHLine(0, ypos, width, color);
	}
	for (coord_t xpos = 0; xpos < width; xpos += 10) {
		lcd_drawVLine(xpos, 0, height, color);
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_drawLine(void) {
	int64_t startTick, endTick, diffTick;

	lcd_fillScreen(BLACK);
	srand((unsigned int)time(NULL));

	startTick = esp_timer_get_time();
	for (int32_t i = 0; i < 100; i++) {
		coord_t x0 = rand() % width;
		coord_t y0 = rand() % height;
		coord_t x1 = rand() % width;
		coord_t y1 = rand() % height;
		lcd_drawLine(x0, y0, x1, y1, RAND_COLOR());
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_drawRect(void) {
	int64_t startTick, endTick, diffTick;

	color_t color = GREEN;
	coord_t limit = width;
	if (width > height) limit = height;
	limit /= 2;
	lcd_fillScreen(BLACK);

	startTick = esp_timer_get_time();
	for (coord_t i = 0; i < limit; i += 5) {
		lcd_drawRect(i, i, width-2*i, height-2*i, color);
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_fillRect(void) {
	int64_t startTick, endTick, diffTick;

	lcd_fillScreen(CYAN);
	srand((unsigned int)time(NULL));

	startTick = esp_timer_get_time();
	for (int32_t i = 0; i < 100; i++) {
		coord_t xpos = rand() % width;
		coord_t ypos = rand() % height;
		coord_t size = rand() % (width/5)+1;
		lcd_fillRect(xpos, ypos, size, size, RAND_COLOR());
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_drawTriangle(void) {
	int64_t startTick, endTick, diffTick;

	lcd_fillScreen(BLACK);
	srand((unsigned int)time(NULL));

	startTick = esp_timer_get_time();
	for (int32_t i = 0; i < 100; i++) {
		coord_t x0 = rand() % width;
		coord_t y0 = rand() % height;
		coord_t x1 = rand() % width;
		coord_t y1 = rand() % height;
		coord_t x2 = rand() % width;
		coord_t y2 = rand() % height;
		lcd_drawTriangle(x0, y0, x1, y1, x2, y2, RAND_COLOR());
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_fillTriangle(void) {
	int64_t startTick, endTick, diffTick;

	lcd_fillScreen(CYAN);
	srand((unsigned int)time(NULL));

	startTick = esp_timer_get_time();
	for (int32_t i = 0; i < 100; i++) {
		coord_t x0 = rand() % width;
		coord_t y0 = rand() % height;
		coord_t x1 = rand() % width;
		coord_t y1 = rand() % height;
		coord_t x2 = rand() % width;
		coord_t y2 = rand() % height;
		lcd_fillTriangle(x0, y0, x1, y1, x2, y2, RAND_COLOR());
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_drawCircle(void) {
	int64_t startTick, endTick, diffTick;

	color_t color = CYAN;
	coord_t limit = width;
	if (height < width) limit = height;
	limit /= 2;
	lcd_fillScreen(BLACK);
	coord_t xpos = width/2;
	coord_t ypos = height/2;

	startTick = esp_timer_get_time();
	for (coord_t i = 5; i < limit; i += 5) {
		lcd_drawCircle(xpos, ypos, i, color);
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_fillCircle(void) {
	int64_t startTick, endTick, diffTick;

	lcd_fillScreen(CYAN);
	srand((unsigned int)time(NULL));

	startTick = esp_timer_get_time();
	for (int32_t i = 0; i < 100; i++) {
		coord_t radius = rand() % (width/5);
		coord_t xpos = rand() % width;
		if (xpos < radius) xpos = radius; // clip
		else if (xpos > width-1-radius) xpos = width-1-radius;
		coord_t ypos = rand() % height;
		if (ypos < radius) ypos = radius; // clip
		else if (ypos > height-1-radius) ypos = height-1-radius;
		lcd_fillCircle(xpos, ypos, radius, RAND_COLOR());
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_drawRoundRect(void) {
	int64_t startTick, endTick, diffTick;

	color_t color = YELLOW;
	coord_t limit = width;
	if (width > height) limit = height;
	limit /= 2;
	lcd_fillScreen(BLACK);

	startTick = esp_timer_get_time();
	for (coord_t i = 0; i < limit; i += 5) {
		lcd_drawRoundRect(i, i, width-2*i, height-2*i, 30, color);
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_fillRoundRect(void) {
	int64_t startTick, endTick, diffTick;

	color_t ctab[] = {YELLOW,rgb565(4, 16, 64)};
	uint8_t c = 0;
	coord_t limit = width;
	if (width > height) limit = height;
	limit /= 2;
	lcd_fillScreen(BLACK);

	startTick = esp_timer_get_time();
	for (coord_t i = 0; i < limit; i += 5) {
		lcd_fillRoundRect(i, i, width-2*i, height-2*i, 30, ctab[c++%2]);
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_drawArrow(void) {
	int64_t startTick, endTick, diffTick;

	color_t color = WHITE;
	coord_t x0 = width/2;
	coord_t y0 = height/2;
	lcd_fillScreen(BLACK);

	startTick = esp_timer_get_time();
	for (coord_t x1 = 0; x1 < width; x1 += 20) {
		lcd_drawArrow(x0, y0, x1, 0, 5, color);
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

#define A_L 15 // Arrow vector length in X & Y
#define A_W  5 // Arrow head half width

int64_t lcd_test_fillArrow(void) {
	int64_t startTick, endTick, diffTick;

	// Get font width & height
	uint8_t fontSize = 2;
	uint8_t fontWidth = LCD_CHAR_W * fontSize;
	uint8_t fontHeight = LCD_CHAR_H * fontSize;

	coord_t xpos;
	coord_t ypos;
	size_t stlen;
	char ascii[24];
	color_t color;

	lcd_fillScreen(BLACK);
	lcd_setFontSize(fontSize);
	lcd_setFontDirection(DIRECTION0);

	startTick = esp_timer_get_time();
	strcpy(ascii, "LCD");
	color = WHITE;
	ypos = ((height - fontHeight) / 2) - 1;
	xpos = (width - (strlen(ascii) * fontWidth)) / 2;
	lcd_drawString(xpos, ypos, ascii, color);

	color = RED;
	lcd_fillArrow(A_L, A_L, 0, 0, A_W, color);
	strcpy(ascii, "0,0");
	ypos = A_L + 8;
	lcd_drawString(0, ypos, ascii, color);

	color = GREEN;
	lcd_fillArrow(width-1-A_L, A_L, width-1, 0, A_W, color);
	sprintf(ascii, "%d,0",(int)width-1);
	stlen = strlen(ascii);
	xpos = (width-1) - (fontWidth*stlen);
	lcd_drawString(xpos, ypos, ascii, color);

	color = GRAY;
	lcd_fillArrow(A_L, height-1-A_L, 0, height-1, A_W, color);
	sprintf(ascii, "0,%d",(int)height-1);
	ypos = (height-1-A_L) - 8 - fontHeight;
	lcd_drawString(0, ypos, ascii, color);

	color = CYAN;
	lcd_fillArrow(width-1-A_L, height-1-A_L, width-1, height-1, A_W, color);
	sprintf(ascii, "%d,%d",(int)width-1, (int)height-1);
	stlen = strlen(ascii);
	xpos = (width-1) - (fontWidth*stlen);
	lcd_drawString(xpos, ypos, ascii, color);
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_drawBitmap(void) {
	int64_t startTick, endTick, diffTick;

	color_t ctab[] = {RED,GREEN,BLUE,BLACK,GRAY,YELLOW,CYAN,MAGENTA};
	lcd_fillScreen(rgb565(4, 16, 64));

	startTick = esp_timer_get_time();
	for (coord_t y = 0; y < LCD_H; y += CROSSHAIR_H+1) {
		coord_t x;
		uint8_t c;
		for (x = 0, c = 0; x < LCD_W; x += CROSSHAIR_W+1, c++) {
			lcd_drawBitmap(x, y, crosshair, CROSSHAIR_W, CROSSHAIR_H, ctab[c%8]);
		}
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_drawRGBBitmap(void) {
	int64_t startTick, endTick, diffTick;
	coord_t x = 0, y = 0;

	startTick = esp_timer_get_time();
	for (; y < 10; y++)
		lcd_drawRGBBitmap(x, y, peppers, PEPPERS_W, PEPPERS_H);
	for (; x < 10; x++)
		lcd_drawRGBBitmap(x, y, peppers, PEPPERS_W, PEPPERS_H);
	for (; y > 0; y--)
		lcd_drawRGBBitmap(x, y, peppers, PEPPERS_W, PEPPERS_H);
	for (; x > 0; x--)
		lcd_drawRGBBitmap(x, y, peppers, PEPPERS_W, PEPPERS_H);
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

//----------------------------------------------------------------------------//
// Rectangle variants that specify two diagonal corners
//----------------------------------------------------------------------------//

int64_t lcd_test_drawRect2(void) {
	int64_t startTick, endTick, diffTick;

	lcd_fillScreen(BLACK);
	srand((unsigned int)time(NULL));

	startTick = esp_timer_get_time();
	for (int32_t i = 0; i < 100; i++) {
		coord_t x0 = rand() % width;
		coord_t y0 = rand() % height;
		coord_t x1 = rand() % width;
		coord_t y1 = rand() % height;
		lcd_drawRect2(x0, y0, x1, y1, RAND_COLOR());
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_fillRect2(void) {
	int64_t startTick, endTick, diffTick;

	lcd_fillScreen(CYAN);
	srand((unsigned int)time(NULL));

	startTick = esp_timer_get_time();
	for (int32_t i = 0; i < 100; i++) {
		coord_t x0 = rand() % width;
		coord_t y0 = rand() % height;
		coord_t x1 = rand() % width;
		coord_t y1 = rand() % height;
		lcd_fillRect2(x0, y0, x1, y1, RAND_COLOR());
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_drawRoundRect2(void) {
	int64_t startTick, endTick, diffTick;

	color_t color = MAGENTA;
	coord_t limit = width;
	if (width > height) limit = height;
	limit /= 2;
	lcd_fillScreen(BLACK);

	startTick = esp_timer_get_time();
	for (coord_t i = 0; i < limit; i += 5) {
		lcd_drawRoundRect2(i, i, width-i-1, height-i-1, 20, color);
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_fillRoundRect2(void) {
	int64_t startTick, endTick, diffTick;

	color_t ctab[] = {MAGENTA,rgb565(4, 16, 64)};
	uint8_t c = 0;
	coord_t limit = width;
	if (width > height) limit = height;
	limit /= 2;
	lcd_fillScreen(BLACK);

	startTick = esp_timer_get_time();
	for (coord_t i = 0; i < limit; i += 5) {
		lcd_fillRoundRect2(i, i, width-i-1, height-i-1, 20, ctab[c++%2]);
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

//----------------------------------------------------------------------------//
// Specify center, size, and rotation angle of primitive shape
//----------------------------------------------------------------------------//

int64_t lcd_test_drawRectC(void) {
	int64_t startTick, endTick, diffTick;

	color_t color = CYAN;
	lcd_fillScreen(BLACK);
	coord_t xpos = width/2;
	coord_t ypos = height/2;
	coord_t h = ((height < width) ? height : width) * 0.7;
	coord_t w = h * 0.5;
	angle_t angle;

	startTick = esp_timer_get_time();
	for (angle = 0; angle < (360*3); angle += 30) {
		lcd_drawRectC(xpos, ypos, w, h, angle, color);
		lcd_drawRectC(xpos, ypos, w, h, angle, BLACK);
	}
	for (angle = 0; angle < 180; angle += 30) {
		lcd_drawRectC(xpos, ypos, w, h, angle, color);
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_drawTriangleC(void) {
	int64_t startTick, endTick, diffTick;

	color_t color = CYAN;
	lcd_fillScreen(BLACK);
	coord_t xpos = width/2;
	coord_t ypos = height/2;
	coord_t h = ((height < width) ? height : width) * 0.7;
	coord_t w = h * 0.7;
	angle_t angle;

	startTick = esp_timer_get_time();
	for (angle = 0; angle < (360*3); angle += 30) {
		lcd_drawTriangleC(xpos, ypos, w, h, angle, color);
		lcd_drawTriangleC(xpos, ypos, w, h, angle, BLACK);
	}
	for (angle = 0; angle < 360; angle += 30) {
		lcd_drawTriangleC(xpos, ypos, w, h, angle, color);
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_drawRegularPolygonC(void) {
	int64_t startTick, endTick, diffTick;

	color_t color = GREEN;
	coord_t xpos = width/2;
	coord_t ypos = height/2;
	coord_t limit = width;
	if (width > height) limit = height;
	limit /= 2;
	lcd_fillScreen(BLACK);

	startTick = esp_timer_get_time();
	for (coord_t n = 3; ; n++) {
		coord_t radius = n*15-35;
		angle_t angle = n*10;
		if (radius >= limit) break;
		lcd_drawRegularPolygonC(xpos, ypos, n, radius, angle, color);
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

//----------------------------------------------------------------------------//
// Draw characters and strings
//----------------------------------------------------------------------------//

// lcd_test_drawChar

int64_t lcd_test_drawString(void) {
	int64_t startTick, endTick, diffTick;

	lcd_fillScreen(BLACK);
	lcd_setFontDirection(DIRECTION0);

	uint8_t size;
	char text[] = "Carpe Diem!";
	size_t tlen = strlen(text);
	color_t bgtab[] = {RED,GREEN,BLUE,BLACK,GRAY,YELLOW,CYAN,MAGENTA};
	srand((unsigned int)time(NULL));

	startTick = esp_timer_get_time();
	for (int32_t i = 0; i < 100; i++) {
		size = (i&0x3)+1;
		coord_t xpos = rand() % (width-LCD_CHAR_W*size*tlen+1);
		coord_t ypos = rand() % (height-LCD_CHAR_H*size+1);
		lcd_setFontSize(size);
		lcd_setFontBackground(bgtab[i%8]);
		lcd_drawString(xpos, ypos, text, RAND_COLOR());
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

//----------------------------------------------------------------------------//
// Font parameters
//----------------------------------------------------------------------------//

int64_t lcd_test_setFontDirection(void) {
	int64_t startTick, endTick, diffTick;

	// get font width & height
	uint8_t fontSize = (LCD_W > 240) ? 3 : 2;

	color_t color;
	char ascii[20];

	lcd_fillScreen(BLACK);
	lcd_setFontSize(fontSize);

	startTick = esp_timer_get_time();
	color = RED;
	strcpy(ascii, "Direction=0");
	lcd_setFontDirection(DIRECTION0);
	lcd_drawString(0, 0, ascii, color);

#if 0 // other directions unimplemented
	color = BLUE;
	strcpy(ascii, "Direction=180");
	lcd_setFontDirection(DIRECTION180);
	lcd_drawString(width-1, height-1, ascii, color);

	color = CYAN;
	strcpy(ascii, "Direction=90");
	lcd_setFontDirection(DIRECTION90);
	lcd_drawString(width-1, 0, ascii, color);

	color = GREEN;
	strcpy(ascii, "Direction=270");
	lcd_setFontDirection(DIRECTION270);
	lcd_drawString(0, height-1, ascii, color);
#endif
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

int64_t lcd_test_setFontSize(void) {
	int64_t startTick, endTick, diffTick;

	uint8_t i;
	coord_t xpos, ypos;
	char ascii[40];
	color_t ctab[] = {RED,WHITE,BLACK,GREEN,BLUE,GRAY,YELLOW,CYAN,MAGENTA};
	lcd_fillScreen(BLACK);
	lcd_setFontDirection(DIRECTION0);

	startTick = esp_timer_get_time();
	for (xpos = 0, ypos = 0, i = 1; ; xpos += 5, ypos += LCD_CHAR_H*i, i++) {
		lcd_setFontSize(i);
		lcd_setFontBackground(ctab[(i+1)%9]);
		sprintf(ascii, "size:%hhu", i);
		if (strlen(ascii)*LCD_CHAR_W*i+xpos > LCD_W) break;
		lcd_drawString(xpos, ypos, ascii, ctab[i%9]);
	}
	endTick = esp_timer_get_time();

	lcd_noFontBackground();
	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

// lcd_test_setFontBackground
// lcd_test_noFontBackground

//----------------------------------------------------------------------------//
// Display configuration
//----------------------------------------------------------------------------//

// lcd_test_spiClockFreq
// lcd_test_displayOff
// lcd_test_displayOn
// lcd_test_backlightOff
// lcd_test_backlightOn
// lcd_test_inversionOff
// lcd_test_inversionOn

//----------------------------------------------------------------------------//
// Frame management
//----------------------------------------------------------------------------//

// lcd_test_frameEnable
// lcd_test_frameDisable
// lcd_test_getFrameBuffer

int64_t lcd_test_wrapAround(void) {
	int64_t startTick, endTick, diffTick;

	if (lcd_getFrameBuffer() == NULL) return 0;
	lcd_drawRGBBitmap(0, 0, peppers, PEPPERS_W, PEPPERS_H);

	startTick = esp_timer_get_time();
	for (coord_t i = 0; i < width/8; i++) {
		lcd_wrapAround(SCROLL_RIGHT, height/4, height/4*3-1); lcd_writeFrame();
	}
	for (coord_t i = 0; i < width/8; i++) {
		lcd_wrapAround(SCROLL_LEFT, height/4, height/4*3-1); lcd_writeFrame();
	}
	for (coord_t i = 0; i < height/8; i++) {
		lcd_wrapAround(SCROLL_DOWN, width/4, width/4*3-1); lcd_writeFrame();
	}
	for (coord_t i = 0; i < height/8; i++) {
		lcd_wrapAround(SCROLL_UP, width/4, width/4*3-1); lcd_writeFrame();
	}
	endTick = esp_timer_get_time();

	lcd_writeFrame();
	diffTick = endTick - startTick;
	PRINT_TIME(diffTick);
	return diffTick;
}

// lcd_test_writeFrame

//----------------------------------------------------------------------------//
// Test all
//----------------------------------------------------------------------------//

void lcd_test_all(void *pvParameters)
{
	lcd_init();
	for (;;) {
		lcd_test_colorBar(); WAIT;
		lcd_test_colorBand(); WAIT;
		lcd_test_fillScreen(); WAIT;
		lcd_test_drawHVLine(); WAIT;
		lcd_test_drawLine(); WAIT;
		lcd_test_drawRect(); WAIT;
		lcd_test_fillRect(); WAIT;
		lcd_test_drawTriangle(); WAIT;
		lcd_test_fillTriangle(); WAIT;
		lcd_test_drawCircle(); WAIT;
		lcd_test_fillCircle(); WAIT;
		lcd_test_drawRoundRect(); WAIT;
		lcd_test_fillRoundRect(); WAIT;
		lcd_test_drawArrow(); WAIT;
		lcd_test_fillArrow(); WAIT;
		lcd_test_drawBitmap(); WAIT;
		lcd_test_drawRGBBitmap(); WAIT;
		lcd_test_drawRect2(); WAIT;
		lcd_test_fillRect2(); WAIT;
		lcd_test_drawRoundRect2(); WAIT;
		lcd_test_fillRoundRect2(); WAIT;
		lcd_test_drawRectC(); WAIT;
		lcd_test_drawTriangleC(); WAIT;
		lcd_test_drawRegularPolygonC(); WAIT;
		lcd_test_drawString(); WAIT;
		lcd_test_setFontDirection(); WAIT;
		lcd_test_setFontSize(); WAIT;
		lcd_test_wrapAround(); WAIT;
		if (lcd_getFrameBuffer() == NULL) lcd_frameEnable();
		else lcd_frameDisable();
	}
}
