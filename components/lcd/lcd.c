/* Modified from: https://github.com/nopnop2002/esp-idf-st7789 */

#include <string.h> // strlen, memcpy
#include <math.h> // cosf, sinf

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#include "lcd.h"

#define TAG "lcd"
#define	_DEBUG_ 0

#ifndef CONFIG_WIDTH
#define CONFIG_WIDTH 320
#endif
#ifndef CONFIG_HEIGHT
#define CONFIG_HEIGHT 240
#endif
#ifndef CONFIG_OFFSETX
#define CONFIG_OFFSETX 0
#endif
#ifndef CONFIG_OFFSETY
#define CONFIG_OFFSETY 0
#endif

#ifndef CONFIG_MOSI_GPIO
#define CONFIG_MOSI_GPIO 23
#endif
#ifndef CONFIG_SCLK_GPIO
#define CONFIG_SCLK_GPIO 18
#endif
#ifndef CONFIG_CS_GPIO
#define CONFIG_CS_GPIO 5
#endif
#ifndef CONFIG_DC_GPIO
#define CONFIG_DC_GPIO 4
#endif
#ifndef CONFIG_RESET_GPIO
#define CONFIG_RESET_GPIO -1
#endif
#ifndef CONFIG_BL_GPIO
#define CONFIG_BL_GPIO 14
#endif

#ifndef CONFIG_INVERSION
#define CONFIG_INVERSION 1
#endif

#if CONFIG_SPI3_HOST
#define HOST_ID SPI3_HOST
#else
#define HOST_ID SPI2_HOST
#endif

#define SPI_DEFAULT_FREQUENCY SPI_MASTER_FREQ_40M; // MHz
#define swap(T,a,b) {T t = (a); (a) = (b); (b) = t;}

#define M_PIf 3.14159265358979323846f

#define SWAP16(c) (((c) << 8) | ((c) >> 8))

static const int32_t SPI_Command_Mode = 0;
static const int32_t SPI_Data_Mode = 1;

static int32_t clock_speed_hz = SPI_DEFAULT_FREQUENCY;

#include "glcdfont.c" // unsigned char font[];


static void delayMS(int32_t ms) {
	int32_t _ms = ms + (portTICK_PERIOD_MS - 1);
	TickType_t xTicksToDelay = _ms / portTICK_PERIOD_MS;
	ESP_LOGD(TAG, "ms=%d _ms=%d portTICK_PERIOD_MS=%"PRIu32" xTicksToDelay=%"PRIu32,(int)ms,(int)_ms,portTICK_PERIOD_MS,xTicksToDelay);
	vTaskDelay(xTicksToDelay);
}

/* * * * * * * * * * SPI * * * * * * * * * */

#define BUF_LEN 512
static uint16_t buffer[BUF_LEN];

static void spi_master_init(TFT_t *dev, int16_t GPIO_MOSI, int16_t GPIO_SCLK, int16_t GPIO_CS, int16_t GPIO_DC, int16_t GPIO_RESET, int16_t GPIO_BL)
{
	esp_err_t ret;

	ESP_LOGI(TAG, "GPIO_CS=%hd",GPIO_CS);
	if ( GPIO_CS >= 0 ) {
		gpio_reset_pin( GPIO_CS );
		gpio_set_direction( GPIO_CS, GPIO_MODE_OUTPUT );
		gpio_set_level( GPIO_CS, 0 );
	}

	ESP_LOGI(TAG, "GPIO_DC=%hd",GPIO_DC);
	gpio_reset_pin( GPIO_DC );
	gpio_set_direction( GPIO_DC, GPIO_MODE_OUTPUT );
	gpio_set_level( GPIO_DC, 0 );

	ESP_LOGI(TAG, "GPIO_RESET=%hd",GPIO_RESET);
	if ( GPIO_RESET >= 0 ) {
		gpio_reset_pin( GPIO_RESET );
		gpio_set_direction( GPIO_RESET, GPIO_MODE_OUTPUT );
		gpio_set_level( GPIO_RESET, 1 );
		delayMS(100);
		gpio_set_level( GPIO_RESET, 0 );
		delayMS(100);
		gpio_set_level( GPIO_RESET, 1 );
		delayMS(100);
	}

	ESP_LOGI(TAG, "GPIO_BL=%hd",GPIO_BL);
	if ( GPIO_BL >= 0 ) {
		gpio_reset_pin(GPIO_BL);
		gpio_set_direction( GPIO_BL, GPIO_MODE_OUTPUT );
		gpio_set_level( GPIO_BL, 0 );
	}

	ESP_LOGI(TAG, "GPIO_MOSI=%hd",GPIO_MOSI);
	ESP_LOGI(TAG, "GPIO_SCLK=%hd",GPIO_SCLK);
	spi_bus_config_t buscfg = {
		.mosi_io_num = GPIO_MOSI,
		.miso_io_num = -1,
		.sclk_io_num = GPIO_SCLK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = 0,
		.flags = 0
	};

	ret = spi_bus_initialize( HOST_ID, &buscfg, SPI_DMA_CH_AUTO );
	ESP_LOGD(TAG, "spi_bus_initialize=%d",(int)ret);
	assert(ret==ESP_OK);

	spi_device_interface_config_t devcfg;
	memset(&devcfg, 0, sizeof(devcfg));
	devcfg.clock_speed_hz = clock_speed_hz;
	devcfg.queue_size = 7;
	devcfg.mode = 3;
	devcfg.flags = SPI_DEVICE_NO_DUMMY;

	if ( GPIO_CS >= 0 ) {
		devcfg.spics_io_num = GPIO_CS;
	} else {
		devcfg.spics_io_num = -1;
	}

	spi_device_handle_t handle;
	ret = spi_bus_add_device( HOST_ID, &devcfg, &handle);
	ESP_LOGD(TAG, "spi_bus_add_device=%d",(int)ret);
	assert(ret==ESP_OK);
	dev->_dc = GPIO_DC;
	dev->_bl = GPIO_BL;
	dev->_SPIHandle = handle;
}

static bool spi_master_write_bytes(spi_device_handle_t SPIHandle, const uint8_t* Data, size_t DataLength)
{
	spi_transaction_t SPITransaction;
	esp_err_t ret;

	if ( DataLength > 0 ) {
		memset( &SPITransaction, 0, sizeof( spi_transaction_t ) );
		SPITransaction.length = DataLength * 8;
		SPITransaction.tx_buffer = Data;
#if 0
		ret = spi_device_transmit( SPIHandle, &SPITransaction );
#else
		ret = spi_device_polling_transmit( SPIHandle, &SPITransaction );
#endif
		assert(ret==ESP_OK);
	}

	return true;
}

static bool spi_master_write_command(TFT_t *dev, uint8_t cmd)
{
	static uint8_t Byte = 0;
	Byte = cmd;
	gpio_set_level( dev->_dc, SPI_Command_Mode );
	return spi_master_write_bytes( dev->_SPIHandle, &Byte, 1 );
}

static bool spi_master_write_data_byte(TFT_t *dev, uint8_t data)
{
	static uint8_t Byte = 0;
	Byte = data;
	gpio_set_level( dev->_dc, SPI_Data_Mode );
	return spi_master_write_bytes( dev->_SPIHandle, &Byte, 1 );
}

#if 0
static bool spi_master_write_data_word(TFT_t *dev, uint16_t data)
{
	static uint8_t Byte[2];
	Byte[0] = (data >> 8) & 0xFF;
	Byte[1] = data & 0xFF;
	gpio_set_level( dev->_dc, SPI_Data_Mode );
	return spi_master_write_bytes( dev->_SPIHandle, Byte, 2);
}
#endif

static bool spi_master_write_addr(TFT_t *dev, uint16_t addr1, uint16_t addr2)
{
	static uint8_t Byte[4];
	Byte[0] = (addr1 >> 8) & 0xFF;
	Byte[1] = addr1 & 0xFF;
	Byte[2] = (addr2 >> 8) & 0xFF;
	Byte[3] = addr2 & 0xFF;
	gpio_set_level( dev->_dc, SPI_Data_Mode );
	return spi_master_write_bytes( dev->_SPIHandle, Byte, 4);
}

// size is number of elements, not bytes.
inline static bool spi_master_write_color(TFT_t *dev, uint16_t color, size_t size)
{
	uint16_t temp = SWAP16(color);
	size_t n = (size < BUF_LEN) ? size : BUF_LEN;
	for (size_t i = 0; i < n; i++) buffer[i] = temp;
	gpio_set_level(dev->_dc, SPI_Data_Mode);
	while (size) {
		n = (size < BUF_LEN) ? size : BUF_LEN;
		spi_master_write_bytes(dev->_SPIHandle, (uint8_t *)buffer, n*sizeof(uint16_t));
		size -= n;
	}
	return true;
}

#if 0 // original
inline static bool spi_master_write_color(TFT_t *dev, uint16_t color, uint16_t size)
{
	static uint8_t Byte[1024];
	int32_t index = 0;
	for(int32_t i=0;i<size;i++) {
		Byte[index++] = (color >> 8) & 0xFF;
		Byte[index++] = color & 0xFF;
	}
	gpio_set_level( dev->_dc, SPI_Data_Mode );
	return spi_master_write_bytes( dev->_SPIHandle, Byte, size*2);
}
#endif

#if 0 // slow
inline static bool spi_master_write_color(TFT_t *dev, uint16_t color, size_t size)
{
	color = SPI_SWAP_DATA_TX(color, 16);
	gpio_set_level( dev->_dc, SPI_Data_Mode );
	while (size--) {
		spi_master_write_bytes( dev->_SPIHandle, (const uint8_t *)&color, 2);
	}
	return 0;
}
#endif

#if 0 // example from: void Adafruit_TFTLCD::pushColors
  uint16_t color;
  uint8_t hi, lo;
  while (len--) {
    color = *data++;
    hi = color >> 8; // Don't simplify or merge these
    lo = color;      // lines, there's macro shenanigans
    write8(hi);      // going on.
    write8(lo);
  }
#endif

#if 0
// Add 202001: size is number of elements, not bytes. size limited to 512
inline static bool spi_master_write_colors(TFT_t *dev, uint16_t *colors, uint16_t size)
{
	static uint8_t Byte[1024];
	int32_t index = 0;
	for(int32_t i=0;i<size;i++) {
		Byte[index++] = (colors[i] >> 8) & 0xFF;
		Byte[index++] = colors[i] & 0xFF;
	}
	gpio_set_level( dev->_dc, SPI_Data_Mode );
	return spi_master_write_bytes( dev->_SPIHandle, Byte, size*2);
}
#endif

// size is number of elements, not bytes.
inline static bool spi_master_write_colors(TFT_t *dev, uint16_t *colors, size_t size)
{
	gpio_set_level(dev->_dc, SPI_Data_Mode);
	while (size) {
		size_t n = (size < BUF_LEN) ? size : BUF_LEN;
		for (size_t i = 0; i < n; i++) buffer[i] = SWAP16(colors[i]);
		spi_master_write_bytes(dev->_SPIHandle, (uint8_t *)buffer, n*sizeof(uint16_t));
		colors += n;
		size -= n;
	}
	return true;
}


/* * * * * * * * * * LCD * * * * * * * * * */

void lcdInit(TFT_t *dev)
{
	spi_master_init(dev,
		CONFIG_MOSI_GPIO,
		CONFIG_SCLK_GPIO,
		CONFIG_CS_GPIO,
		CONFIG_DC_GPIO,
		CONFIG_RESET_GPIO,
		CONFIG_BL_GPIO);

	dev->_width = CONFIG_WIDTH;
	dev->_height = CONFIG_HEIGHT;
	dev->_offsetx = CONFIG_OFFSETX;
	dev->_offsety = CONFIG_OFFSETY;
	dev->_font_direction = DIRECTION0;
	dev->_font_size = 1;
	dev->_font_back_en = false;
	dev->_font_back_color = BLACK;
	dev->_use_frame_buffer = false;
	dev->_frame_buffer = NULL;

	spi_master_write_command(dev, 0x01);	// ILI:Software Reset (01h), ST:SWRESET (01h): Software Reset
	delayMS(5); // 150

	spi_master_write_command(dev, 0x11);	// ILI:Sleep Out (11h), ST:SLPOUT (11h): Sleep Out
	delayMS(5); // 255

	spi_master_write_command(dev, 0x3A);	// ILI:COLMOD: Pixel Format Set (3Ah), ST:COLMOD (3Ah): Interface Pixel Format
	spi_master_write_data_byte(dev, 0x55);
	// delayMS(10);

	spi_master_write_command(dev, 0x36);	// ILI:Memory Access Control (36h), ST:MADCTL (36h): Memory Data Access Control
	spi_master_write_data_byte(dev, 0x08);  // 0x00

	// spi_master_write_command(dev, 0x2A);	// ILI:Column Address Set (2Ah), ST:CASET (2Ah): Column Address Set
	// spi_master_write_data_byte(dev, 0x00);
	// spi_master_write_data_byte(dev, 0x00);
	// spi_master_write_data_byte(dev, 0x00);
	// spi_master_write_data_byte(dev, 0xF0);
	// spi_master_write_addr(dev, 0, width-1);

	// spi_master_write_command(dev, 0x2B);	// ILI:Page Address Set (2Bh), ST:RASET (2Bh): Row Address Set
	// spi_master_write_data_byte(dev, 0x00);
	// spi_master_write_data_byte(dev, 0x00);
	// spi_master_write_data_byte(dev, 0x00);
	// spi_master_write_data_byte(dev, 0xF0);
	// spi_master_write_addr(dev, 0, height-1);

	spi_master_write_command(dev, 0xCF);    // ILI:Power control B (CFh), ILI9341 only
	spi_master_write_data_byte(dev, 0x00);
	spi_master_write_data_byte(dev, 0xc3);
	spi_master_write_data_byte(dev, 0x30);
	spi_master_write_command(dev, 0xED);    // ILI:Power on sequence control (EDh), ILI9341 only
	spi_master_write_data_byte(dev, 0x64);
	spi_master_write_data_byte(dev, 0x03);
	spi_master_write_data_byte(dev, 0x12);
	spi_master_write_data_byte(dev, 0x81);
	spi_master_write_command(dev, 0xE8);    // ILI:Driver timing control A (E8h), ST:PWCTRL2 (E8h): Power Control 2
	spi_master_write_data_byte(dev, 0x85);
	spi_master_write_data_byte(dev, 0x00);
	spi_master_write_data_byte(dev, 0x78);
	spi_master_write_command(dev, 0xCB);    // ILI:Power control A (CBh), ILI9341 only
	spi_master_write_data_byte(dev, 0x39);
	spi_master_write_data_byte(dev, 0x2c);
	spi_master_write_data_byte(dev, 0x00);
	spi_master_write_data_byte(dev, 0x34);
	spi_master_write_data_byte(dev, 0x02);
	spi_master_write_command(dev, 0xF7);    // ILI:Pump ratio control (F7h), ILI9341 only
	spi_master_write_data_byte(dev, 0x20);
	spi_master_write_command(dev, 0xEA);    // ILI:Driver timing control B (EAh), ILI9341 only
	spi_master_write_data_byte(dev, 0x00);
	spi_master_write_data_byte(dev, 0x00);
	spi_master_write_command(dev, 0xC0);    // ILI:Power Control 1 (C0h), ST:LCMCTRL (C0h): LCM Control
	spi_master_write_data_byte(dev, 0x1B);
	spi_master_write_command(dev, 0xC1);    // ILI:Power Control 2 (C1h), ST:IDSET (C1h): ID Code Setting
	spi_master_write_data_byte(dev, 0x12);
	spi_master_write_command(dev, 0xC5);	// ILI:VCOM Control 1(C5h), ST:VCMOFSET (C5h): VCOM Offset Set
	spi_master_write_data_byte(dev, 0x32);
	spi_master_write_data_byte(dev, 0x3C);
	spi_master_write_command(dev, 0xC7);	// ILI:VCOM Control 2(C7h), ST:CABCCTRL (C7h): CABC Control
	spi_master_write_data_byte(dev, 0x91);
	spi_master_write_command(dev, 0xB1);	// ILI:Frame Rate Control (In Normal Mode/Full Colors) (B1h), ST:RGBCTRL (B1h): RGB Interface Control
	spi_master_write_data_byte(dev, 0x00);
	spi_master_write_data_byte(dev, 0x10);
	spi_master_write_command(dev, 0xB6);	// ILI:Display Function Control (B6h), ILI9341 only
	spi_master_write_data_byte(dev, 0x0A);
	spi_master_write_data_byte(dev, 0xA2);
	spi_master_write_command(dev, 0xF6);    // ILI:Interface Control (F6h), ILI9341 only
	spi_master_write_data_byte(dev, 0x01);
	spi_master_write_data_byte(dev, 0x30);

#if CONFIG_INVERSION
	ESP_LOGI(TAG, "Enable Display Inversion");
	lcdInversionOn(dev);
#else
	lcdInversionOff(dev);
#endif
	delayMS(10);

	// spi_master_write_command(dev, 0x13);	//Normal Display Mode On
	// delayMS(10);

	spi_master_write_command(dev, 0x29);	//Display ON
	lcdFillScreen(dev, BLACK); // assume _use_frame_buffer is false
	// delayMS(255);

	if(dev->_bl >= 0) {
		gpio_set_level( dev->_bl, 1 );
	}
}

// Fill screen
// color:color
void lcdFillScreen(TFT_t *dev, uint16_t color) {
	if (dev->_use_frame_buffer) {
		uint16_t *ptr = dev->_frame_buffer;
		size_t len = dev->_width*dev->_height;
		*ptr++ = color; len--;
		while (len) {
			size_t n = (len < ptr - dev->_frame_buffer) ? len : ptr - dev->_frame_buffer;
			memcpy(ptr, dev->_frame_buffer, n*sizeof(uint16_t));
			ptr += n; len -= n;
		}
	} else {
		spi_master_write_command(dev, 0x2A);	// set column(x) address
		spi_master_write_addr(dev, 0, dev->_width-1);
		spi_master_write_command(dev, 0x2B);	// set Page(y) address
		spi_master_write_addr(dev, 0, dev->_height-1);
		spi_master_write_command(dev, 0x2C);	// Memory Write
		spi_master_write_color(dev, color, dev->_width*dev->_height);
	}
}

// Draw pixel
// x:X coordinate
// y:Y coordinate
// color:color
void lcdDrawPixel(TFT_t *dev, int32_t x, int32_t y, uint16_t color){
	if (x < 0 || x >= dev->_width) return; // off screen
	if (y < 0 || y >= dev->_height) return;

	if (dev->_use_frame_buffer) {
		dev->_frame_buffer[y*dev->_width+x] = color;
	} else {
		int32_t _x = x + dev->_offsetx;
		int32_t _y = y + dev->_offsety;

		spi_master_write_command(dev, 0x2A);	// set column(x) address
		spi_master_write_addr(dev, _x, _x);
		spi_master_write_command(dev, 0x2B);	// set Page(y) address
		spi_master_write_addr(dev, _y, _y);
		spi_master_write_command(dev, 0x2C);	// Memory Write
		//spi_master_write_data_word(dev, color);
		spi_master_write_colors(dev, &color, 1);
	}
}

// Draw multi pixel
// x:X coordinate
// y:Y coordinate
// size:Number of colors
// colors:colors
void lcdDrawMultiPixels(TFT_t *dev, int32_t x, int32_t y, int32_t size, uint16_t *colors) {
	if (x+size <= 0 || x >= dev->_width) return; // off screen
	if (y < 0 || y >= dev->_height) return;
	if (x < 0) {size += x; x = 0;} // clip
	if (x+size > dev->_width) size = dev->_width-x;

	if (dev->_use_frame_buffer) {
		int32_t _x1 = x;
		int32_t _x2 = _x1 + (size-1);
		int32_t index = 0;
		for(int32_t i = _x1; i <= _x2; i++){
			dev->_frame_buffer[y*dev->_width+i] = colors[index++];
		}
	} else {
		int32_t _x1 = x + dev->_offsetx;
		int32_t _x2 = _x1 + (size-1);
		int32_t _y1 = y + dev->_offsety;
		int32_t _y2 = _y1;

		spi_master_write_command(dev, 0x2A);	// set column(x) address
		spi_master_write_addr(dev, _x1, _x2);
		spi_master_write_command(dev, 0x2B);	// set Page(y) address
		spi_master_write_addr(dev, _y1, _y2);
		spi_master_write_command(dev, 0x2C);	// Memory Write
		spi_master_write_colors(dev, colors, size);
	}
}

// Draw Horizontal Line
// x:X coordinate
// y:Y coordinate
// w:width of line
// color:color
void lcdDrawHLine(TFT_t *dev, int32_t x, int32_t y, int32_t w, uint16_t color) {
	if (x+w <= 0 || x >= dev->_width) return; // off screen
	if (y < 0 || y >= dev->_height) return;
	if (x < 0) {w += x; x = 0;} // clip
	if (x+w > dev->_width) w = dev->_width-x;

	if (dev->_use_frame_buffer) {
		int32_t _x1 = x;
		int32_t _x2 = _x1 + (w-1);
		for(int32_t i = _x1; i <= _x2; i++){
			dev->_frame_buffer[y*dev->_width+i] = color;
		}
	} else {
		int32_t _x1 = x + dev->_offsetx;
		int32_t _x2 = _x1 + (w-1);
		int32_t _y1 = y + dev->_offsety;
		int32_t _y2 = _y1;

		spi_master_write_command(dev, 0x2A);	// set column(x) address
		spi_master_write_addr(dev, _x1, _x2);
		spi_master_write_command(dev, 0x2B);	// set Page(y) address
		spi_master_write_addr(dev, _y1, _y2);
		spi_master_write_command(dev, 0x2C);	// Memory Write
		spi_master_write_color(dev, color, w);
	}
}

// Draw Vertical Line
// x:X coordinate
// y:Y coordinate
// h:height of line
// color:color
void lcdDrawVLine(TFT_t *dev, int32_t x, int32_t y, int32_t h, uint16_t color) {
	int32_t y2 = y+h-1;
	if (x < 0 || x  >= dev->_width) return; // off screen
	if (y2 < 0 || y >= dev->_height) return;
	if (y < 0) y = 0; // clip
	if (y2 >= dev->_height) y2 = dev->_height-1;

	ESP_LOGD(TAG,"offset(x)=%ld offset(y)=%ld",dev->_offsetx,dev->_offsety);

	if (dev->_use_frame_buffer) {
		for (int32_t j = y; j <= y2; j++){
			dev->_frame_buffer[j*dev->_width+x] = color;
		}
	} else {
		int32_t _x1 =  x  + dev->_offsetx;
		int32_t _x2 = _x1 + dev->_offsetx;
		int32_t _y1 =  y  + dev->_offsety;
		int32_t _y2 =  y2 + dev->_offsety;
		int32_t size = _y2-_y1+1;

		spi_master_write_command(dev, 0x2A);	// set column(x) address
		spi_master_write_addr(dev, _x1, _x2);
		spi_master_write_command(dev, 0x2B);	// set Page(y) address
		spi_master_write_addr(dev, _y1, _y2);
		spi_master_write_command(dev, 0x2C);	// Memory Write
		spi_master_write_color(dev, color, size);
	}
}

#if 0
// Draw line
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End	X coordinate
// y2:End	Y coordinate
// color:color
void lcdDrawLine(TFT_t *dev, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color) {
	int32_t i;
	int32_t dx, dy;
	int32_t sx, sy;
	int32_t E;

	/* distance between two points */
	dx = ( x2 > x1 ) ? x2 - x1 : x1 - x2;
	dy = ( y2 > y1 ) ? y2 - y1 : y1 - y2;

	/* direction of two point */
	sx = ( x2 > x1 ) ? 1 : -1;
	sy = ( y2 > y1 ) ? 1 : -1;

	/* inclination < 1 */
	if ( dx > dy ) {
		E = -dx;
		for ( i = 0 ; i <= dx ; i++ ) {
			lcdDrawPixel(dev, x1, y1, color);
			x1 += sx;
			E += 2 * dy;
			if ( E >= 0 ) {
			y1 += sy;
			E -= 2 * dx;
		}
	}

	/* inclination >= 1 */
	} else {
		E = -dy;
		for ( i = 0 ; i <= dy ; i++ ) {
			lcdDrawPixel(dev, x1, y1, color);
			y1 += sy;
			E += 2 * dx;
			if ( E >= 0 ) {
				x1 += sx;
				E -= 2 * dy;
			}
		}
	}
}
#endif

#if 0
// Draw line
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End	X coordinate
// y2:End	Y coordinate
// color:color
// Bresenham's algorithm - thx Wikipedia
void lcdDrawLine(TFT_t *dev, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color) {
  bool steep = abs(y1 - y0) > abs(x1 - x0);

  if (steep) {
    swap(int32_t, x0, y0);
    swap(int32_t, x1, y1);
  }

  if (x0 > x1) {
    swap(int32_t, x0, x1);
    swap(int32_t, y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0 <= x1; x0++) {
    if (steep) {
      lcdDrawPixel(dev, y0, x0, color);
    } else {
      lcdDrawPixel(dev, x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}
#endif

/***************************************************************************************
** Function name:           lcdDrawLine
** Description:             draw a line between 2 arbitrary points
***************************************************************************************/
// Bresenham's algorithm - thx Wikipedia - speed enhanced by Bodmer to use
// efficient H/V Line draw routines for line segments of 2 pixels or more.
void lcdDrawLine(TFT_t *dev, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t color)
{
  bool steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(int32_t, x0, y0);
    swap(int32_t, x1, y1);
  }

  if (x0 > x1) {
    swap(int32_t, x0, x1);
    swap(int32_t, y0, y1);
  }

  int32_t dx = x1 - x0, dy = abs(y1 - y0);;

  int32_t err = dx >> 1, ystep = -1, xs = x0, dlen = 0;

  if (y0 < y1) ystep = 1;

  // Split into steep and not steep for FastH/V separation
  if (steep) {
    for (; x0 <= x1; x0++) {
      dlen++;
      err -= dy;
      if (err < 0) {
        if (dlen == 1) lcdDrawPixel(dev, y0, xs, color);
        else lcdDrawVLine(dev, y0, xs, dlen, color);
        dlen = 0;
        y0 += ystep; xs = x0 + 1;
        err += dx;
      }
    }
    if (dlen) lcdDrawVLine(dev, y0, xs, dlen, color);
  }
  else
  {
    for (; x0 <= x1; x0++) {
      dlen++;
      err -= dy;
      if (err < 0) {
        if (dlen == 1) lcdDrawPixel(dev, xs, y0, color);
        else lcdDrawHLine(dev, xs, y0, dlen, color);
        dlen = 0;
        y0 += ystep; xs = x0 + 1;
        err += dx;
      }
    }
    if (dlen) lcdDrawHLine(dev, xs, y0, dlen, color);
  }
}

// Draw rectangle - assume x1 <= x2 && y1 <= y2
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End	X coordinate
// y2:End	Y coordinate
// color:color
void lcdDrawRect(TFT_t *dev, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color) {
#if 1
	lcdDrawHLine(dev, x1, y1, x2-x1+1, color);
	lcdDrawVLine(dev, x2, y1, y2-y1+1, color);
	lcdDrawHLine(dev, x1, y2, x2-x1+1, color);
	lcdDrawVLine(dev, x1, y1, y2-y1+1, color);
#else
	lcdDrawLine(dev, x1, y1, x2, y1, color);
	lcdDrawLine(dev, x2, y1, x2, y2, color);
	lcdDrawLine(dev, x2, y2, x1, y2, color);
	lcdDrawLine(dev, x1, y2, x1, y1, color);
#endif
}

// Draw rectangle of filling - assume x1 <= x2 && y1 <= y2
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End X coordinate
// y2:End Y coordinate
// color:color
void lcdFillRect(TFT_t *dev, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color) {
	if (x2 < 0 || x1 >= dev->_width) return; // off screen
	if (y2 < 0 || y1 >= dev->_height) return;
	if (x1 < 0) x1 = 0; // clip
	if (x2 >= dev->_width) x2=dev->_width-1;
	if (y1 < 0) y1 = 0;
	if (y2 >= dev->_height) y2=dev->_height-1;

	ESP_LOGD(TAG,"offset(x)=%ld offset(y)=%ld",dev->_offsetx,dev->_offsety);

	if (dev->_use_frame_buffer) {
		for (int32_t j = y1; j <= y2; j++){
			for(int32_t i = x1; i <= x2; i++){
				dev->_frame_buffer[j*dev->_width+i] = color;
			}
		}
	} else {
		int32_t _x1 = x1 + dev->_offsetx;
		int32_t _x2 = x2 + dev->_offsetx;
		int32_t _y1 = y1 + dev->_offsety;
		int32_t _y2 = y2 + dev->_offsety;
		int32_t size = (_x2-_x1+1)*(_y2-_y1+1);

		spi_master_write_command(dev, 0x2A);	// set column(x) address
		spi_master_write_addr(dev, _x1, _x2);
		spi_master_write_command(dev, 0x2B);	// set Page(y) address
		spi_master_write_addr(dev, _y1, _y2);
		spi_master_write_command(dev, 0x2C);	// Memory Write
		spi_master_write_color(dev, color, size);
	}
}

/***************************************************************************************
** Function name:           lcdDrawTri
** Description:             Draw a triangle outline using 3 arbitrary points
***************************************************************************************/
// Draw a triangle
void lcdDrawTri(TFT_t *dev, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color)
{
  lcdDrawLine(dev, x0, y0, x1, y1, color);
  lcdDrawLine(dev, x1, y1, x2, y2, color);
  lcdDrawLine(dev, x2, y2, x0, y0, color);
}

/***************************************************************************************
** Function name:           lcdFillTri
** Description:             Draw a filled triangle using 3 arbitrary points
***************************************************************************************/
// Fill a triangle - original Adafruit function works well and code footprint is small
void lcdFillTri(TFT_t *dev, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint16_t color)
{
  int32_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    swap(int32_t, y0, y1); swap(int32_t, x0, x1);
  }
  if (y1 > y2) {
    swap(int32_t, y2, y1); swap(int32_t, x2, x1);
  }
  if (y0 > y1) {
    swap(int32_t, y0, y1); swap(int32_t, x0, x1);
  }

  if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if (x1 < a)      a = x1;
    else if (x1 > b) b = x1;
    if (x2 < a)      a = x2;
    else if (x2 > b) b = x2;
	lcdDrawHLine(dev, a, y0, b - a + 1, color);
    return;
  }

  int32_t
  dx01 = x1 - x0,
  dy01 = y1 - y0,
  dx02 = x2 - x0,
  dy02 = y2 - y0,
  dx12 = x2 - x1,
  dy12 = y2 - y1,
  sa   = 0,
  sb   = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if (y1 == y2) last = y1;  // Include y1 scanline
  else          last = y1 - 1; // Skip it

  for (y = y0; y <= last; y++) {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;

    if (a > b) swap(int32_t, a, b);
	lcdDrawHLine(dev, a, y, b - a + 1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for (; y <= y2; y++) {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;

    if (a > b) swap(int32_t, a, b);
	lcdDrawHLine(dev, a, y, b - a + 1, color);
  }
}

// Draw circle
// x0:Central X coordinate
// y0:Central Y coordinate
// r:radius
// color:color
void lcdDrawCircle(TFT_t *dev, int32_t x0, int32_t y0, int32_t r, uint16_t color) {
	int32_t x;
	int32_t y;
	int32_t err;
	int32_t old_err;

	x=0;
	y=-r;
	err=2-2*r;
	do{
		lcdDrawPixel(dev, x0-x, y0+y, color);
		lcdDrawPixel(dev, x0-y, y0-x, color);
		lcdDrawPixel(dev, x0+x, y0-y, color);
		lcdDrawPixel(dev, x0+y, y0+x, color);
		if ((old_err=err)<=x)	err+=++x*2+1;
		if (old_err>y || err>x) err+=++y*2+1;
	} while(y<0);
}

// Draw circle of filling
// x0:Central X coordinate
// y0:Central Y coordinate
// r:radius
// color:color
void lcdFillCircle(TFT_t *dev, int32_t x0, int32_t y0, int32_t r, uint16_t color) {
	int32_t x;
	int32_t y;
	int32_t err;
	int32_t old_err;
	int32_t ChangeX;

	x=0;
	y=-r;
	err=2-2*r;
	ChangeX=1;
	do{
		if(ChangeX) {
#if 1
			lcdDrawVLine(dev, x0-x, y0+y, (-y<<1)+1, color);
			lcdDrawVLine(dev, x0+x, y0+y, (-y<<1)+1, color);
#else
			lcdDrawLine(dev, x0-x, y0-y, x0-x, y0+y, color);
			lcdDrawLine(dev, x0+x, y0-y, x0+x, y0+y, color);
#endif
		} // endif
		ChangeX=(old_err=err)<=x;
		if (ChangeX)			err+=++x*2+1;
		if (old_err>y || err>x) err+=++y*2+1;
	} while(y<=0);
}

// Draw rectangle with round corner
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End	X coordinate
// y2:End	Y coordinate
// r:radius
// color:color
void lcdDrawRoundRect(TFT_t *dev, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t r, uint16_t color) {
	int32_t x;
	int32_t y;
	int32_t err;
	int32_t old_err;

	if(x1>x2) swap(int32_t, x1, x2);
	if(y1>y2) swap(int32_t, y1, y2);

	ESP_LOGD(TAG, "x1=%ld x2=%ld delta=%ld r=%ld",x1, x2, x2-x1, r);
	ESP_LOGD(TAG, "y1=%ld y2=%ld delta=%ld r=%ld",y1, y2, y2-y1, r);
	int32_t w = x2-x1+1-(r<<1);
	int32_t h = y2-y1+1-(r<<1);
	if (w < 1 || h < 1) return;

	x=0;
	y=-r;
	err=2-2*r;

	do{
		if(x) {
			lcdDrawPixel(dev, x1+r-x, y1+r+y, color);
			lcdDrawPixel(dev, x2-r+x, y1+r+y, color);
			lcdDrawPixel(dev, x1+r-x, y2-r-y, color);
			lcdDrawPixel(dev, x2-r+x, y2-r-y, color);
		} // endif
		if ((old_err=err)<=x)	err+=++x*2+1;
		if (old_err>y || err>x) err+=++y*2+1;
	} while(y<0);
#if 1
	ESP_LOGD(TAG, "x1+r=%ld x2-r=%ld",x1+r, x2-r);
	lcdDrawHLine(dev, x1+r,y1  , w, color);
	lcdDrawHLine(dev, x1+r,y2  , w, color);
	ESP_LOGD(TAG, "y1+r=%ld y2-r=%ld",y1+r, y2-r);
	lcdDrawVLine(dev, x1  ,y1+r, h, color);
	lcdDrawVLine(dev, x2  ,y1+r, h, color);
#else
	ESP_LOGD(TAG, "x1+r=%ld x2-r=%ld",x1+r, x2-r);
	lcdDrawLine(dev, x1+r,y1  ,x2-r,y1	, color);
	lcdDrawLine(dev, x1+r,y2  ,x2-r,y2	, color);
	ESP_LOGD(TAG, "y1+r=%ld y2-r=%ld",y1+r, y2-r);
	lcdDrawLine(dev, x1  ,y1+r,x1  ,y2-r, color);
	lcdDrawLine(dev, x2  ,y1+r,x2  ,y2-r, color);
#endif
}

// Draw arrow
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End	X coordinate
// y2:End	Y coordinate
// w:Width of the botom
// color:color
// Thanks http://k-hiura.cocolog-nifty.com/blog/2010/11/post-2a62.html
void lcdDrawArrow(TFT_t *dev, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, uint16_t color) {
	float Vx = x1 - x0;
	float Vy = y1 - y0;
	float v = sqrtf(Vx*Vx+Vy*Vy);
	//	 printf("v=%f\n",v);
	float Ux= Vx/v;
	float Uy= Vy/v;

	int32_t L[2],R[2];
	L[0]= x1 - Uy*w - Ux*v;
	L[1]= y1 + Ux*w - Uy*v;
	R[0]= x1 + Uy*w - Ux*v;
	R[1]= y1 - Ux*w - Uy*v;
	//printf("L=%ld-%ld R=%ld-%ld\n",L[0],L[1],R[0],R[1]);

	//lcdDrawLine(x0,y0,x1,y1,color);
	lcdDrawLine(dev, x1, y1, L[0], L[1], color);
	lcdDrawLine(dev, x1, y1, R[0], R[1], color);
	lcdDrawLine(dev, L[0], L[1], R[0], R[1], color);
}


// Draw arrow of filling
// x1:Start X coordinate
// y1:Start Y coordinate
// x2:End	X coordinate
// y2:End	Y coordinate
// w:Width of the botom
// color:color
void lcdFillArrow(TFT_t *dev, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, uint16_t color) {
	float Vx = x1 - x0;
	float Vy = y1 - y0;
	float v = sqrtf(Vx*Vx+Vy*Vy);
	//printf("v=%f\n",v);
	float Ux= Vx/v;
	float Uy= Vy/v;

	int32_t L[2],R[2];
	L[0]= x1 - Uy*w - Ux*v;
	L[1]= y1 + Ux*w - Uy*v;
	R[0]= x1 + Uy*w - Ux*v;
	R[1]= y1 - Ux*w - Uy*v;
	//printf("L=%ld-%ld R=%ld-%ld\n",L[0],L[1],R[0],R[1]);

	lcdDrawLine(dev, x0, y0, x1, y1, color);
	lcdDrawLine(dev, x1, y1, L[0], L[1], color);
	lcdDrawLine(dev, x1, y1, R[0], R[1], color);
	lcdDrawLine(dev, L[0], L[1], R[0], R[1], color);

	int32_t ww;
	for(ww=w-1;ww>0;ww--) {
		L[0]= x1 - Uy*ww - Ux*v;
		L[1]= y1 + Ux*ww - Uy*v;
		R[0]= x1 + Uy*ww - Ux*v;
		R[1]= y1 - Ux*ww - Uy*v;
		//printf("Fill>L=%ld-%ld R=%ld-%ld\n",L[0],L[1],R[0],R[1]);
		lcdDrawLine(dev, x1, y1, L[0], L[1], color);
		lcdDrawLine(dev, x1, y1, R[0], R[1], color);
	}
}

// Draw rectangle with angle
// xc:Center X coordinate
// yc:Center Y coordinate
// w:Width of rectangle
// h:Height of rectangle
// angle:Angle of rectangle
// color:color

// When the origin is (0, 0), the point (x1, y1) after rotating the point (x, y)
// by the angle is obtained by the following calculation.
// x1 = x * cos(angle) - y * sin(angle)
// y1 = x * sin(angle) + y * cos(angle)
void lcdDrawRectangle(TFT_t *dev, int32_t xc, int32_t yc, int32_t w, int32_t h, int32_t angle, uint16_t color) {
	float xd, yd, rd;
	int32_t x1, y1;
	int32_t x2, y2;
	int32_t x3, y3;
	int32_t x4, y4;
	rd = -angle * M_PIf / 180.0f;
	xd = 0.0f - w/2;
	yd = h/2;
	x1 = (int32_t)(xd * cosf(rd) - yd * sinf(rd) + xc);
	y1 = (int32_t)(xd * sinf(rd) + yd * cosf(rd) + yc);

	yd = 0.0f - yd;
	x2 = (int32_t)(xd * cosf(rd) - yd * sinf(rd) + xc);
	y2 = (int32_t)(xd * sinf(rd) + yd * cosf(rd) + yc);

	xd = w/2;
	yd = h/2;
	x3 = (int32_t)(xd * cosf(rd) - yd * sinf(rd) + xc);
	y3 = (int32_t)(xd * sinf(rd) + yd * cosf(rd) + yc);

	yd = 0.0f - yd;
	x4 = (int32_t)(xd * cosf(rd) - yd * sinf(rd) + xc);
	y4 = (int32_t)(xd * sinf(rd) + yd * cosf(rd) + yc);

	lcdDrawLine(dev, x1, y1, x2, y2, color);
	lcdDrawLine(dev, x1, y1, x3, y3, color);
	lcdDrawLine(dev, x2, y2, x4, y4, color);
	lcdDrawLine(dev, x3, y3, x4, y4, color);
}

// Draw triangle
// xc:Center X coordinate
// yc:Center Y coordinate
// w:Width of triangle
// h:Height of triangle
// angle:Angle of triangle
// color:color

// When the origin is (0, 0), the point (x1, y1) after rotating the point (x, y)
// by the angle is obtained by the following calculation.
// x1 = x * cos(angle) - y * sin(angle)
// y1 = x * sin(angle) + y * cos(angle)
void lcdDrawTriangle(TFT_t *dev, int32_t xc, int32_t yc, int32_t w, int32_t h, int32_t angle, uint16_t color) {
	float xd, yd, rd;
	int32_t x1, y1;
	int32_t x2, y2;
	int32_t x3, y3;
	rd = -angle * M_PIf / 180.0f;
	xd = 0.0f;
	yd = h/2;
	x1 = (int32_t)(xd * cosf(rd) - yd * sinf(rd) + xc);
	y1 = (int32_t)(xd * sinf(rd) + yd * cosf(rd) + yc);

	xd = w/2;
	yd = 0.0f - yd;
	x2 = (int32_t)(xd * cosf(rd) - yd * sinf(rd) + xc);
	y2 = (int32_t)(xd * sinf(rd) + yd * cosf(rd) + yc);

	xd = 0.0f - w/2;
	x3 = (int32_t)(xd * cosf(rd) - yd * sinf(rd) + xc);
	y3 = (int32_t)(xd * sinf(rd) + yd * cosf(rd) + yc);

	lcdDrawLine(dev, x1, y1, x2, y2, color);
	lcdDrawLine(dev, x1, y1, x3, y3, color);
	lcdDrawLine(dev, x2, y2, x3, y3, color);
}

// Draw regular polygon
// xc:Center X coordinate
// yc:Center Y coordinate
// n:Number of slides
// r:radius
// angle:Angle of regular polygon
// color:color
void lcdDrawRegularPolygon(TFT_t *dev, int32_t xc, int32_t yc, int32_t n, int32_t r, int32_t angle, uint16_t color)
{
	float xd, yd, rd;
	int32_t x1, y1;
	int32_t x2, y2;
	int32_t i;

	rd = -angle * M_PIf / 180.0f;
	for (i = 0; i < n; i++)	{
		xd = r * cosf(2 * M_PIf * i / n);
		yd = r * sinf(2 * M_PIf * i / n);
		x1 = (int32_t)(xd * cosf(rd) - yd * sinf(rd) + xc);
		y1 = (int32_t)(xd * sinf(rd) + yd * cosf(rd) + yc);

		xd = r * cosf(2 * M_PIf * (i + 1) / n);
		yd = r * sinf(2 * M_PIf * (i + 1) / n);
		x2 = (int32_t)(xd * cosf(rd) - yd * sinf(rd) + xc);
		y2 = (int32_t)(xd * sinf(rd) + yd * cosf(rd) + yc);

		lcdDrawLine(dev, x1, y1, x2, y2, color);
	}
}

// Draw ASCII character
// x:X coordinate
// y:Y coordinate
// ascii: ascii code
// color:color
int32_t lcdDrawChar(TFT_t *dev, int32_t x, int32_t y, char ascii, uint16_t color) {
#if 0
  if ((x >= dev->_width) ||                        // off screen right
      (y >= dev->_height) ||                       // off screen bottom
      ((x + LCD_CHAR_W * dev->_font_size) <= 0) || // off screen left
      ((y + LCD_CHAR_H * dev->_font_size) <= 0))   // off screen top
    return;
#endif

  if (dev->_font_back_en) {
    lcdFillRect(dev, x, y,
      x+LCD_CHAR_W*dev->_font_size-1,
      y+LCD_CHAR_H*dev->_font_size-1,
      dev->_font_back_color);
  }
  for (int8_t i = 0; i < LCD_CHAR_W; i++) {
    uint8_t line;
    if (i == LCD_CHAR_W-1)
      line = 0x0;
    else
      line = font[((uint8_t)ascii * (LCD_CHAR_W-1)) + i];
    for (int8_t j = 0; j < LCD_CHAR_H; j++) {
      if (line & 0x1) {
        if (dev->_font_size == 1) // default size
          lcdDrawPixel(dev, x + i, y + j, color);
        else { // big size
          int32_t x1 = x + (i * dev->_font_size), y1 = y + (j * dev->_font_size);		
          lcdFillRect(dev, x1, y1, x1+dev->_font_size-1, y1+dev->_font_size-1, color);
        }
      }
#if 0
      else if (dev->_font_back_en) {
        if (dev->_font_size == 1) // default size
          lcdDrawPixel(dev, x + i, y + j, dev->_font_back_color);
        else { // big size
          int32_t x1 = x + (i * dev->_font_size), y1 = y + (j * dev->_font_size);		
          lcdFillRect(dev, x1, y1, x1+dev->_font_size-1, y1+dev->_font_size-1, dev->_font_back_color);
        }
      }
#endif
      line >>= 1;
    }
  }
  return x+LCD_CHAR_W*dev->_font_size;
}

// Draw ASCII string
// x:X coordinate
// y:Y coordinate
// ascii: ascii string, zero terminated
// color:color
int32_t lcdDrawString(TFT_t *dev, int32_t x, int32_t y, char *ascii, uint16_t color) {
	int32_t length = strlen(ascii);
	for (int32_t i=0; i<length; i++) {
		x = lcdDrawChar(dev, x, y, ascii[i], color);
	}
	return x;
}

// Set font direction
// dir:Direction
void lcdSetFontDirection(TFT_t *dev, direction_t dir) {
	dev->_font_direction = dir;
}

// Set font size
// size: font size
void lcdSetFontSize(TFT_t *dev, uint8_t size) {
	dev->_font_size = size;
}

// Set font background
// color:background color
void lcdSetFontBackground(TFT_t *dev, uint16_t color) {
	dev->_font_back_en = true;
	dev->_font_back_color = color;
}

// No font background
void lcdNoFontBackground(TFT_t *dev) {
	dev->_font_back_en = false;
}

// Set display SPI clock
void lcdSPIClockSpeed(int32_t speed) {
    ESP_LOGI(TAG, "SPI clock speed=%d MHz", (int)speed/1000000);
    clock_speed_hz = speed;
}

// Display OFF
void lcdDisplayOff(TFT_t *dev) {
	spi_master_write_command(dev, 0x28);	// Display off
}

// Display ON
void lcdDisplayOn(TFT_t *dev) {
	spi_master_write_command(dev, 0x29);	// Display on
}

// Backlight OFF
void lcdBacklightOff(TFT_t *dev) {
	if(dev->_bl >= 0) {
		gpio_set_level( dev->_bl, 0 );
	}
}

// Backlight ON
void lcdBacklightOn(TFT_t *dev) {
	if(dev->_bl >= 0) {
		gpio_set_level( dev->_bl, 1 );
	}
}

// Display Inversion Off
void lcdInversionOff(TFT_t *dev) {
	spi_master_write_command(dev, 0x20); // Display Inversion Off
}

// Display Inversion On
void lcdInversionOn(TFT_t *dev) {
	spi_master_write_command(dev, 0x21); // Display Inversion On
}

// Enable use of frame buffer
void lcdFrameEnable(TFT_t *dev) {
	dev->_frame_buffer = heap_caps_malloc(sizeof(uint16_t)*dev->_width*dev->_height, MALLOC_CAP_DMA);
	if (dev->_frame_buffer == NULL) {
		ESP_LOGE(TAG, "heap_caps_malloc fail");
	} else {
		ESP_LOGI(TAG, "heap_caps_malloc success");
		dev->_use_frame_buffer = true;
	}
}

// Disable use of frame buffer
void lcdFrameDisable(TFT_t *dev) {
	if (dev->_frame_buffer != NULL) heap_caps_free(dev->_frame_buffer);
	dev->_use_frame_buffer = false;
}

// Scroll image in frame buffer
void lcdWrapArround(TFT_t *dev, scroll_t scroll, int32_t start, int32_t end) {
	if (dev->_use_frame_buffer == false) return;

	int32_t _width = dev->_width;
	int32_t _height = dev->_height;
	int32_t index1;
	int32_t index2;

	if (scroll == SCROLL_RIGHT) {
		uint16_t wk[_width];
		for (int32_t i=start;i<end;i++) {
			index1 = i * _width;
			memcpy((char *)wk, (char*)&dev->_frame_buffer[index1], _width*2);
			index2 = index1 + _width - 1;
			dev->_frame_buffer[index1] = dev->_frame_buffer[index2];
			memcpy((char *)&dev->_frame_buffer[index1+1], (char *)&wk[0], (_width-1)*2);
		}
	} else if (scroll == SCROLL_LEFT) {
		uint16_t wk[_width];
		for (int32_t i=start;i<end;i++) {
			index1 = i * _width;
			memcpy((char *)wk, (char*)&dev->_frame_buffer[index1], _width*2);
			index2 = index1 + _width - 1;
			dev->_frame_buffer[index2] = dev->_frame_buffer[index1];
			memcpy((char *)&dev->_frame_buffer[index1], (char *)&wk[1], (_width-1)*2);
		}
	} else if (scroll == SCROLL_UP) {
		uint16_t wk;
		for (int32_t i=start;i<=end;i++) {
			wk = dev->_frame_buffer[i];
			for (int32_t j=0;j<_height-1;j++) {
				index1 = j * _width + i;
				index2 = (j+1) * _width + i;
				dev->_frame_buffer[index1] = dev->_frame_buffer[index2];
			}
			index2 = (_height-1) * _width + i;
			dev->_frame_buffer[index2] = wk;
		}
	} else if (scroll == SCROLL_DOWN) {
		uint16_t wk;
		for (int32_t i=start;i<=end;i++) {
			index2 = (_height-1) * _width + i;
			wk = dev->_frame_buffer[index2];
			for (int32_t j=_height-2;j>=0;j--) {
				index1 = j * _width + i;
				index2 = (j+1) * _width + i;
				dev->_frame_buffer[index2] = dev->_frame_buffer[index1];
			}
			dev->_frame_buffer[i] = wk;
		}
	}
}

// Write frame buffer to display
void lcdWriteFrame(TFT_t *dev)
{
	if (dev->_use_frame_buffer == false) return;

	spi_master_write_command(dev, 0x2A); // set column(x) address
	spi_master_write_addr(dev, dev->_offsetx, dev->_offsetx+dev->_width-1);
	spi_master_write_command(dev, 0x2B); // set Page(y) address
	spi_master_write_addr(dev, dev->_offsety, dev->_offsety+dev->_height-1);
	spi_master_write_command(dev, 0x2C); // Memory Write
	spi_master_write_colors(dev, dev->_frame_buffer, dev->_width*dev->_height);

#if 0
	size_t size = dev->_width*dev->_height;
	uint16_t *image = dev->_frame_buffer;
	while (size > 0) {
		// 1024 bytes per time.
		size_t bs = (size > 512) ? 512 : size;
		spi_master_write_colors(dev, image, bs);
		size -= bs;
		image += bs;
	}
#endif
	return;
}
