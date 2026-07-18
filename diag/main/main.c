#include <stdio.h> // sprintf
#include <stdint.h>
#include <stdbool.h>
#include <string.h> // strlen

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h" // ESP_LOG*
#include "esp_timer.h" // esp_timer_get_time
#include "driver/gpio.h" // gpio_*, gpio_dump_io_configuration

#include "hw.h" // HW_*
#include "lcd.h" // LCD_*, lcd_*, rgb565()
#include "joy.h" // JOY_MAX_DISP, joy_get_displacement
#include "tone.h" // tone_*

static const char *TAG = "diag";

#define CHK_RET(x) ({                                       \
    int32_t ret_val = (x);                                  \
    if (ret_val != 0) {                                     \
        ESP_LOGE(TAG, "FAIL: return %ld, %s", ret_val, #x); \
    }                                                       \
    ret_val;                                                \
})

// Timer support
#define UP_PER 30 // Update period in ms
#define TIME_OUT 500 // ms

static TimerHandle_t update_timer;

/******************** GPIO ********************/

// GPIO support
#define PIN_MAX  40
#define PIN_COLS 8
#define PIN_ROWS 3
#define PIN_W    (LCD_W/PIN_COLS)
#define PIN_H    (LCD_H/PIN_ROWS)
#define PIN_R    (PIN_W/4)

#define LGAP 2 // label gap

#define IN_CLR GREEN
#define OUT_CLR rgb565(255,127,255)
#define GRP_CLR CYAN // Group color
#define LABEL_CLR YELLOW
#define PIN_CLR WHITE
#define STA_CLR GREEN // Status color

#define MODE_X (LCD_CHAR_W*15)
#define MODE_Y 0

typedef struct {
	const char *label;
	gpio_num_t pin;
	bool in;
	bool out;
} pin_conf_t;

static pin_conf_t external[] = {
	{    "EX8", HW_EX8,        false, true },
	{    "EX7", HW_EX7,        false, true },
	{    "EX6", HW_EX6,        false, true },
	{    "EX5", HW_EX5,        false, true },
	{    "EX4", HW_EX4,        false, true },
};
static pin_conf_t buttons[] = {
	{   "Menu", HW_BTN_MENU,    true, false },
	{ "Option", HW_BTN_OPTION,  true, false },
	{       "", -1,            false, false },
	{ "Select", HW_BTN_SELECT,  true, false },
	{  "Start", HW_BTN_START,   true, false },
	{       "", -1,            false, false },
	{      "B", HW_BTN_B,       true, false },
	{      "A", HW_BTN_A,       true, false },
};

typedef struct {
	coord_t x;
	coord_t y;
	const char *label;
	pin_conf_t *ptr;
	gpio_num_t len;
} group_t;

static const group_t grp_list[] = {
	{0, PIN_H*0, "External I/O", external, sizeof(external)/sizeof(pin_conf_t)},
	{0, PIN_H*2, "Buttons",      buttons,  sizeof(buttons)/sizeof(pin_conf_t)},
};

static bool pin_state[PIN_MAX];
static enum {OUT, IN} mode;
static uint32_t io_cnt, io_num;

// Initialize I/O pins in a configuration group.
static void io_init_grp(const pin_conf_t *grp, gpio_num_t len)
{
	for (gpio_num_t i = 0; i < len; i++) { // Each pin in group
		gpio_num_t pin = grp[i].pin;
		if (pin < 0) continue;
		bool out = grp[i].out;
		gpio_reset_pin(pin);
		if (pin < GPIO_NUM_34 || pin > GPIO_NUM_39) // Skip if no pullup
			gpio_set_pull_mode(pin, out ? GPIO_FLOATING:GPIO_PULLUP_ONLY);
		// If output, set as input in addition so it can be read
		gpio_set_direction(pin, out ? GPIO_MODE_INPUT_OUTPUT:GPIO_MODE_INPUT);
		if (out) gpio_set_level(pin, 0);
		pin_state[pin] = !gpio_get_level(pin); // force a redraw
		// gpio_dump_io_configuration(stdout, 1LLU << pin);
	}
}

// Initialize all I/O pins in each configuration group
static void io_init_all(void)
{
	for (int32_t i = 0; i < sizeof(grp_list)/sizeof(group_t); i++) {
		const group_t *g = grp_list + i;
		io_init_grp(g->ptr, g->len);
	}
}

// Draw a group of I/O pins in a row.
static void io_draw_grp(coord_t xa, coord_t ya, const pin_conf_t *grp, gpio_num_t len, bool init)
{
	coord_t yc = ya+PIN_H/2;
	coord_t y1 = yc - (PIN_R+1+LGAP) - (LCD_CHAR_H-1);
	coord_t y2 = yc + (PIN_R+1+LGAP);
	for (gpio_num_t i = 0; i < len; i++) { // Each pin in group
		if (grp[i].pin < 0) continue;
		coord_t xc = xa+i*PIN_W+PIN_W/2;
		coord_t x1 = xc-(LCD_CHAR_W*strlen(grp[i].label))/2;
		coord_t x2 = xc-(LCD_CHAR_W*2);
		gpio_num_t pin = grp[i].pin;
		color_t color1 = grp[i].out ? OUT_CLR : IN_CLR;
		color_t color0 = BLACK;
		if (init) {
			lcd_drawCircle(xc, yc, PIN_R, color1);
			lcd_drawString(x1, y1, grp[i].label, LABEL_CLR);
			char str[8];
			sprintf(str, "IO%02hhd", pin);
			lcd_drawString(x2, y2, str, PIN_CLR);
		}
		int32_t cps = gpio_get_level(pin); // Current pin state
		if (init || cps != pin_state[pin]) {
			// if (!init) ESP_LOGI(TAG, "IO%02u=%u", (unsigned)pin, (unsigned)cps);
			lcd_fillCircle(xc, yc, PIN_R-1, cps ? color1 : color0);
			pin_state[pin] = cps;
		}
	}
}

// Draw a panel indicating the state of the I/O pins on the display.
static void io_draw_all(bool init)
{
	for (int32_t i = 0; i < sizeof(grp_list)/sizeof(group_t); i++) {
		const group_t *g = grp_list + i;
		if (init) lcd_drawString(g->x, g->y, g->label, GRP_CLR);
		io_draw_grp(g->x, g->y, g->ptr, g->len, init);
	}
}

// Setup the io
static void io_setup(void)
{
	mode = OUT;
	io_cnt = 0;
	io_num = 0;
	io_init_all();
	io_draw_all(true);
	lcd_drawString(MODE_X, MODE_Y, "Mode (Select):", LABEL_CLR);
	lcd_drawString(MODE_X+LCD_CHAR_W*14, MODE_Y, mode?" in":"out", STA_CLR);
}

// Update the io information
static void io_update(void)
{
	static bool btnSELECT = false;
	const group_t *g = grp_list + 0; // External I/O group

	// Handle btnSELECT and status "in/out"
	if (!btnSELECT && !gpio_get_level(HW_BTN_SELECT)) {
		btnSELECT = true;
		mode = !mode; // Toggle I/O mode and update pin color
		coord_t yc = g->y+PIN_H/2;
		for (uint8_t i = 0; i < g->len; i++) {
			coord_t xc = g->x+i*PIN_W+PIN_W/2;
			pin_conf_t *pconf = g->ptr + i;
			if (pconf->pin < 0) continue;
			pconf->in = !pconf->in;
			pconf->out = !pconf->out;
			lcd_drawCircle(xc, yc, PIN_R, pconf->out ? OUT_CLR : IN_CLR);
		}
		io_init_grp(g->ptr, g->len);
		lcd_drawString(MODE_X+LCD_CHAR_W*14, MODE_Y, mode?" in":"out", STA_CLR);
		return; // skip redraw until next tick, reduce WCET
	} else if (btnSELECT && gpio_get_level(HW_BTN_SELECT)) {
		btnSELECT = false;
	} else if (mode == OUT) {
		// Set each external output high for a pulse when in out mode
		if (!(io_cnt & 0x1F)) {
			io_num = 0;
			if (io_num < g->len) gpio_set_level(g->ptr[io_num].pin, 1);
		} else if (!(io_cnt & 0x03)) {
			if (io_num < g->len) gpio_set_level(g->ptr[io_num].pin, 0);
			io_num++;
			if (io_num < g->len) gpio_set_level(g->ptr[io_num].pin, 1);
		}
		io_cnt++;
	}
	io_draw_all(false);
}

/******************** Joystick ********************/

// Cursor support
#define CUR_SZ 7 // Cursor size
#define CUR_CLR WHITE // Cursor color
#define SBG_CLR BLACK // Screen background color

// Cursor viewport
#define VIEW_X (LCD_W/2-PIN_H/2)
#define VIEW_Y (LCD_H/2-PIN_H/2)
#define VIEW_W PIN_H
#define VIEW_H PIN_H

// Cursor limits used to calculate overlap with viewport
#define VIEW_X1 (VIEW_X+CUR_SZ/2+1)
#define VIEW_Y1 (VIEW_Y+CUR_SZ/2+1)
#define VIEW_X2 (VIEW_X+VIEW_W-CUR_SZ/2-1)
#define VIEW_Y2 (VIEW_Y+VIEW_H-CUR_SZ/2-1)

#define OV 0 // Over scale amount in pixels
#define A2X(d) (((d)*(PIN_H/2+OV))/JOY_MAX_DISP+(LCD_W/2))
#define A2Y(d) (((d)*(PIN_H/2+OV))/JOY_MAX_DISP+(LCD_H/2))
#define CLIP(x,lo,hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))

static coord_t dcx, dcy;

// Draw the joystick status
static void joystick_status(void)
{
	char str[16];

	sprintf(str, "(%5ld,%5ld)", dcx, dcy);
	lcd_drawString(0, PIN_H*1+LCD_CHAR_H*3+LGAP, str, STA_CLR);
}

// Draw the cursor on the display
static void cursor(coord_t x, coord_t y, color_t color)
{
	lcd_drawHLine(x-(CUR_SZ/2), y,            CUR_SZ, color);
	lcd_drawVLine(x,            y-(CUR_SZ/2), CUR_SZ, color);
}

// Setup the cursor
static void cursor_setup(void)
{
	// Initialize joystick driver
	CHK_RET(joy_init());
	// Draw group label
	lcd_drawString(0, PIN_H*1, "Joystick", GRP_CLR);
	// Draw field label
	lcd_drawString(0, PIN_H*1+LCD_CHAR_H*2, "     X     Y", LABEL_CLR);
	// Draw box around cursor area
	lcd_drawRect(VIEW_X, VIEW_Y, VIEW_W, VIEW_H, CUR_CLR);
}

// Update the cursor position
static void cursor_update(void)
{
	static coord_t lx = -CUR_SZ, ly = -CUR_SZ;
	coord_t x, y;

	joy_get_displacement(&dcx, &dcy);
	// Convert from joystick displacement to screen coordinates
	x = A2X(dcx); y = A2Y(dcy);
	x = CLIP(x, VIEW_X-CUR_SZ/2-1, VIEW_X+VIEW_W+CUR_SZ/2);
	y = CLIP(y, VIEW_Y-CUR_SZ/2-1, VIEW_Y+VIEW_H+CUR_SZ/2);
	if (x != lx || y != ly) { // Update cursor position
		cursor(lx, ly, SBG_CLR);
		if (lx < VIEW_X1 || ly < VIEW_Y1 || lx >= VIEW_X2 || ly >= VIEW_Y2) {
			// Refresh cursor border
			lcd_drawRect(VIEW_X, VIEW_Y, VIEW_W, VIEW_H, CUR_CLR);
		}
		cursor(x, y, CUR_CLR);
		lx = x; ly = y;
		joystick_status();
	}
}

/******************** Sound ********************/

#define SND_X (PIN_W*6-LCD_CHAR_W*3)
#define SND_Y (PIN_H*1)
#define VOL_INC 20 // %
#define SAMPLE_RATE 24000 // Hz
#define VOLUME_DEFAULT (VOL_INC*3) // %

// Musical note frequencies
#define A3 220
#define A4 440
#define A5 880

static uint32_t vol;
static tone_t tone;

// Draw the sound status
static void sound_status(void)
{
	lcd_drawString(SND_X+LCD_CHAR_W*13, SND_Y+LCD_CHAR_H*2, tone_busy()?" on":"off", STA_CLR);
	static char *ttab[] = {"sin", "squ", "tri", "saw"};
	lcd_drawString(SND_X+LCD_CHAR_W*13, SND_Y+LCD_CHAR_H*3, ttab[tone], STA_CLR);
	char str[12];
	sprintf(str, "%3lu", vol);
	lcd_drawString(SND_X+LCD_CHAR_W*13, SND_Y+LCD_CHAR_H*4, str, STA_CLR);
}

// Setup the sound
static void sound_setup(void)
{
	// Initialize tone and sample rate
	CHK_RET(tone_init(SAMPLE_RATE));
	vol = VOLUME_DEFAULT;
	tone = SINE_T;
	tone_set_volume(vol);
	// Draw static text
	lcd_drawString(SND_X, SND_Y, "Sound", GRP_CLR);
	lcd_drawString(SND_X, SND_Y+LCD_CHAR_H*2, "Play (Btn A):", LABEL_CLR);
	lcd_drawString(SND_X, SND_Y+LCD_CHAR_H*3, "Tone  (Menu):", LABEL_CLR);
	lcd_drawString(SND_X, SND_Y+LCD_CHAR_H*4, "Vol (Option):", LABEL_CLR);
	sound_status();
}

// Update the sound information
static void sound_update(void)
{
	static bool btnA = false, btnMENU = false, btnOPT = false;

	if (!btnA && !gpio_get_level(HW_BTN_A)) {
		btnA = true;
		tone_start(tone, A4);
		lcd_drawString(SND_X+LCD_CHAR_W*13, SND_Y+LCD_CHAR_H*2, " on", STA_CLR);
	} else if (btnA && gpio_get_level(HW_BTN_A)) {
		btnA = false;
		tone_stop();
		lcd_drawString(SND_X+LCD_CHAR_W*13, SND_Y+LCD_CHAR_H*2, "off", STA_CLR);
	}
	if (!btnMENU && !gpio_get_level(HW_BTN_MENU)) {
		btnMENU = true;
		tone = (tone+1)%LAST_T;
		if (tone_busy()) tone_start(tone, A4); // update tone if playing
		static char *ttab[] = {"sin", "squ", "tri", "saw"};
		lcd_drawString(SND_X+LCD_CHAR_W*13, SND_Y+LCD_CHAR_H*3, ttab[tone], STA_CLR);
	} else if (btnMENU && gpio_get_level(HW_BTN_MENU)) {
		btnMENU = false;
	}
	if (!btnOPT && !gpio_get_level(HW_BTN_OPTION)) {
		btnOPT = true;
		vol = (vol <= MAX_VOL-VOL_INC) ? vol+VOL_INC : 0;
		tone_set_volume(vol);
		char str[12];
		sprintf(str, "%3lu", vol);
		lcd_drawString(SND_X+LCD_CHAR_W*13, SND_Y+LCD_CHAR_H*4, str, STA_CLR);
	} else if (btnOPT && gpio_get_level(HW_BTN_OPTION)) {
		btnOPT = false;
	}
}

/******************** Application ********************/

#define WCET_X1 (LCD_W-LCD_CHAR_W*15)
#define WCET_X2 (WCET_X1+LCD_CHAR_W*8)
#define WCET_Y 0

static uint64_t tmax = 0; // Worst Case Execution Time (WCET)

// Tick function for application
static void update(TimerHandle_t xTimer)
{
	uint64_t t1 = esp_timer_get_time();
	io_update();
	cursor_update();
	sound_update();
	uint64_t t2 = esp_timer_get_time() - t1;
	if (t2 > tmax) {
		tmax = t2;
		char str[22];
		sprintf(str, "%llu", tmax);
		lcd_drawString(WCET_X2, WCET_Y, str, STA_CLR);
	}
}

// Main application.
void app_main(void)
{
	ESP_LOGI(TAG, "Start up");
	lcd_init();
	lcd_fillScreen(SBG_CLR);
	lcd_setFontBackground(SBG_CLR); // Set font background
	lcd_drawString(WCET_X1, WCET_Y, "WCET us:", LABEL_CLR);

	io_setup();
	cursor_setup();
	sound_setup();

	// Schedule the tick() function to run at the specified period.
	// The joy_get_displacement() function is not safe to call from an
	// ISR context. Therefore, the update() function must be called from
	// a software task context provided by the RTOS.
	update_timer = xTimerCreate(
		"update_timer",        // Text name for the timer.
		pdMS_TO_TICKS(UP_PER), // The timer period in ticks.
		pdTRUE,                // Auto-reload the timer when it expires.
		NULL,                  // No need for a timer ID.
		update                 // Tick function (callback) when timer expires.
	);
	if (update_timer == NULL) {
		ESP_LOGE(TAG, "Error creating timer");
		return;
	}
	if (xTimerStart(update_timer, pdMS_TO_TICKS(TIME_OUT)) != pdPASS) {
		ESP_LOGE(TAG, "Error starting timer");
		return;
	}

	vTaskSuspend(NULL);
}
