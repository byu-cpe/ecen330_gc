
#include "lcd.h"
#include "watch.h"

#define IN_MARG  30 // inner margin between face and digits
#define OUT_MARG  8 // outer margin between face and display edge
#define CLK_CHR   8 // number of clock characters
#define FNT_SZ   (((LCD_W-2*IN_MARG-2*OUT_MARG)/CLK_CHR)/LCD_CHAR_W)
#define CHR_W    (LCD_CHAR_W*FNT_SZ)

// Clock digits dimensions
#define CLK_W (CHR_W*CLK_CHR)
#define CLK_H (LCD_CHAR_H*FNT_SZ)
#define CLK_X ((LCD_W-CLK_W)/2)
#define CLK_Y ((LCD_H-CLK_H)/2)
#define CLK_C YELLOW // clock digits color

// Display face dimensions
#define FACE_EX (CLK_X-OUT_MARG) // expansion of face around digits
#define FACE_X1 (CLK_X-FACE_EX)
#define FACE_Y1 (CLK_Y-FACE_EX)
#define FACE_X2 (CLK_X+CLK_W-1+FACE_EX)
#define FACE_Y2 (CLK_Y+CLK_H-1+FACE_EX)
#define FACE_R  ((FACE_Y2-FACE_Y1+1)/4)
#define FACE_C  WHITE // face color
#define FACE_BG BLACK // face background color

#define ANN_XM  (CLK_X+CHR_W*1-LCD_CHAR_W*3/2) // minutes annotation x position
#define ANN_XS  (CLK_X+CHR_W*4-LCD_CHAR_W*3/2) // seconds annotation x position
#define ANN_Y   (CLK_Y+CLK_H+LCD_CHAR_H) // annotation y position
#define ANN_C   rgb565( 50, 220,  40) // annotation color

#define BASE6   6
#define BASE10 10


// Initialize the watch face.
void watch_init(void)
{
	// Clear display with background color
	lcd_fillScreen(FACE_BG);
	lcd_setFontDirection(DIRECTION0);
	lcd_setFontBackground(FACE_BG);

	// Watch face
	lcd_drawRoundRect2(FACE_X1, FACE_Y1, FACE_X2, FACE_Y2, FACE_R, FACE_C);

	// Watch face annotation
	lcd_setFontSize(1);
	lcd_drawString(ANN_XM, ANN_Y, "MIN", ANN_C);
	lcd_drawString(ANN_XS, ANN_Y, "SEC", ANN_C);

	// Set clock digit font size
	lcd_setFontSize(FNT_SZ);
}

// Update the watch digits based on timer_ticks (1/100th of a second).
void watch_update(uint32_t timer_ticks)
{
	static uint32_t last_ticks = -1;
	static char last_digits[] = "XX*XX*XX";
	char curr_digits[CLK_CHR];

	if (timer_ticks == last_ticks) return;
	last_ticks = timer_ticks;

	// Convert timer_ticks (1/100th second) into watch digits
	curr_digits[7] = timer_ticks % BASE10 + '0'; // 1/100 sec
	timer_ticks /= BASE10;
	curr_digits[6] = timer_ticks % BASE10 + '0'; // 1/10 sec
	timer_ticks /= BASE10;
	curr_digits[5] = '.';
	curr_digits[4] = timer_ticks % BASE10 + '0'; // 1 sec
	timer_ticks /= BASE10;
	curr_digits[3] = timer_ticks % BASE6  + '0'; // 10 sec
	timer_ticks /= BASE6;
	curr_digits[2] = ':';
	curr_digits[1] = timer_ticks % BASE10 + '0'; // 1 min
	timer_ticks /= BASE10;
	curr_digits[0] = timer_ticks % BASE10 + '0'; // 10 min

	// Update digits if changed
	for (int32_t i = 0; i < CLK_CHR; i++) {
		if (curr_digits[i] != last_digits[i]) {
			lcd_drawChar(CLK_X+i*CHR_W, CLK_Y, curr_digits[i], CLK_C);
			last_digits[i] = curr_digits[i];
		}
	}
}
