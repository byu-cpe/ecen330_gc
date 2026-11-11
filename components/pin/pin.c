#include <stdio.h>
#include "soc/reg_base.h" // DR_REG_GPIO_BASE, DR_REG_IO_MUX_BASE
#include "driver/rtc_io.h" // rtc_gpio_*
#include "pin.h"

#define GPIO_OUT_REG          (DR_REG_GPIO_BASE+0x04)
#define GPIO_OUT_W1TS_REG       (DR_REG_GPIO_BASE+0x08)
#define GPIO_OUT_W1TC_REG      (DR_REG_GPIO_BASE+0x0C)
#define GPIO_OUT1_REG          (DR_REG_GPIO_BASE+0x10)
#define GPIO_ENABLE_REG      (DR_REG_GPIO_BASE+0x20)
#define GPIO_ENABLE_W1TS_REG      (DR_REG_GPIO_BASE+0x24)
#define GPIO_ENABLE_W1TC_REG      (DR_REG_GPIO_BASE+0x28)
#define GPIO_ENABLE1_REG      (DR_REG_GPIO_BASE+0x2C)
#define GPIO_ENABLE1_W1TS_REG      (DR_REG_GPIO_BASE+0x30)
#define GPIO_ENABLE1_W1TC_REG      (DR_REG_GPIO_BASE+0x34)
#define GPIO_STRAP_REG      (DR_REG_GPIO_BASE+0x1C)
#define GPIO_IN_REG       (DR_REG_GPIO_BASE+0x3C)
#define GPIO_IN1_REG       (DR_REG_GPIO_BASE+0x40)
#define GPIO_STATUS_REG       (DR_REG_GPIO_BASE+0x24)
#define GPIO_STATUS_W1TS_REG       (DR_REG_GPIO_BASE+0x28)
#define GPIO_STATUS_W1TC_REG       (DR_REG_GPIO_BASE+0x2C)
#define GPIO_ACPU_INT_REG       (DR_REG_GPIO_BASE+0x30)
#define GPIO_PCPU_INT_REG       (DR_REG_GPIO_BASE+0x34)
#define GPIO_PCPU_NMI_INT_REG       (DR_REG_GPIO_BASE+0x38)
#define GPIO_PIN_REG(n)       (DR_REG_GPIO_BASE+0x88+((n)*4))
#define GPIO_FUNC_OUT_SEL_CFG_REG  (DR_REG_GPIO_BASE+0x530)
#define IO_MUX_REG(n) (PIN_MUX_REG_OFFSET[n] + DR_REG_IO_MUX_BASE)
#define FUN_WPD  7
#define FUN_WPU  8
#define MCU_SEL 12
#define FUN_DRV 10
#define FUN_IE 9
#define PAD_DRIVER 2
#define REG(r) (*(volatile uint32_t *)(r))
#define REG_BITS 32
#define REG_SET_BIT(r,b) REG(r) |= (1U << (b)) 
#define REG_CLR_BIT(r,b) REG(r) &= ~(1U << (b)) 
#define REG_GET_BIT(r,b) ((REG(r) >> (b)) & (1U))  

// Gives byte offset of IO_MUX Configuration Register
// from base address DR_REG_IO_MUX_BASE
static const uint8_t PIN_MUX_REG_OFFSET[] = {
    0x44, 0x88, 0x40, 0x84, 0x48, 0x6c, 0x60, 0x64, // pin  0- 7
    0x68, 0x54, 0x58, 0x5c, 0x34, 0x38, 0x30, 0x3c, // pin  8-15
    0x4c, 0x50, 0x70, 0x74, 0x78, 0x7c, 0x80, 0x8c, // pin 16-23
    0x90, 0x24, 0x28, 0x2c, 0xFF, 0xFF, 0xFF, 0xFF, // pin 24-31
    0x1c, 0x20, 0x14, 0x18, 0x04, 0x08, 0x0c, 0x10, // pin 32-39
};

// Reset the configuration of a pin to not be an input or an output.
// Pull-up is enabled so the pin does not float.
// Return zero if successful, or non-zero otherwise.
int32_t pin_reset(pin_num_t pin)
{
	if (rtc_gpio_is_valid_gpio(pin)) { // hand-off work to RTC subsystem
		rtc_gpio_deinit(pin);
		rtc_gpio_pullup_en(pin);
		rtc_gpio_pulldown_dis(pin);
	}
	REG(GPIO_PIN_REG(pin)) = 0;
	REG(GPIO_FUNC_OUT_SEL_CFG_REG) = 0x100;
	REG(IO_MUX_REG(pin)) = (2<<MCU_SEL| 2<<FUN_DRV | 1<<FUN_WPU);


	return pin_set_level(pin, 0);
}

// Enable or disable a pull-up on the pin.
// Return zero if successful, or non-zero otherwise.
int32_t pin_pullup(pin_num_t pin, bool enable)
{
	if (rtc_gpio_is_valid_gpio(pin)) { // hand-off work to RTC subsystem
		if (enable) return rtc_gpio_pullup_en(pin);
		else return rtc_gpio_pullup_dis(pin);
	}
	//sets and clears the FUN_WPU pin in the IO_MUX register
	if (enable)
	{
		REG_SET_BIT(IO_MUX_REG(pin), FUN_WPU);
	}
	else
	{
		REG_CLR_BIT(IO_MUX_REG(pin), FUN_WPU);
	}
	return 0;
}

// Enable or disable a pull-down on the pin.
// Return zero if successful, or non-zero otherwise.
int32_t pin_pulldown(pin_num_t pin, bool enable)
{	
	// hand-off work to RTC subsystem
	if (rtc_gpio_is_valid_gpio(pin)) { 
		if (enable) return rtc_gpio_pulldown_en(pin);
		else return rtc_gpio_pulldown_dis(pin);
	}
	//Sets and clears the FUN_WPD bit in an IO_MUX register
	if (enable)
	{
		REG_CLR_BIT(IO_MUX_REG(pin), FUN_WPD);
	}
	else
	{
		REG_SET_BIT(IO_MUX_REG(pin), FUN_WPD);
	}
	return 0;
}

// Enable or disable the pin as an input signal.
// Return zero if successful, or non-zero otherwise.
int32_t pin_input(pin_num_t pin, bool enable)
{
	//Sets and clears the FUN_IE bit in an IO_MUX register
	if (enable)
	{
		REG_SET_BIT(IO_MUX_REG(pin), FUN_IE);
	}
	else
	{
		if (pin < REG_BITS)
		{
			REG_CLR_BIT(IO_MUX_REG(pin), FUN_IE);
		}
	}
	return 0;
}

// Enable or disable the pin as an output signal.
// Return zero if successful, or non-zero otherwise.
int32_t pin_output(pin_num_t pin, bool enable)
{
	// Sets and clears the I/O pin bit in the ENABLE or ENABLE1 register
	if (pin < REG_BITS)
	{	
		if (enable)
		{
			REG_SET_BIT(GPIO_ENABLE_REG, pin -REG_BITS);
		}
		else
		{
			REG_CLR_BIT(GPIO_ENABLE_REG, pin -REG_BITS);
		}
		
	}
	else
	{
		if (enable)
		{
			REG_SET_BIT(GPIO_ENABLE1_REG, pin -REG_BITS);
		}
		else
		{
			REG_CLR_BIT(GPIO_ENABLE1_REG, pin-REG_BITS);
		}
	}
	return 0;
	
}

// Enable or disable the 7pin as an open-drain signal.
// Return zero if successful, or non-zero otherwise.
int32_t pin_odrain(pin_num_t pin, bool enable)
{
	// Sets and clears the PAD_DRIVER bit in a PIN register
	if(enable)
	{
		REG_SET_BIT(GPIO_PIN_REG(pin), PAD_DRIVER);
	}
	else
	{
		REG_CLR_BIT(GPIO_PIN_REG(pin), PAD_DRIVER);
	}
	return 0;
}

// Sets the output signal level if the pin is configured as an output.
// Return zero if successful, or non-zero otherwise.
int32_t pin_set_level(pin_num_t pin, int32_t level)
{
	//Sets and clears the I/O pin bit in the OUT or OUT1 register
	if (pin < REG_BITS)
	{
		if (level)
		{
			REG_SET_BIT(GPIO_OUT_REG, pin);
		}
		else
		{
			REG_CLR_BIT(GPIO_OUT_REG, pin);
		}
	}
	else
	{
		if (level)
		{
			REG_SET_BIT(GPIO_OUT1_REG,(pin-REG_BITS));
		}
		else
		{
			REG_CLR_BIT(GPIO_OUT1_REG,(pin-REG_BITS));
		}
	}
	return 0;
}

// Gets the input signal level if the pin is configured as an input.
// Return zero or one if successful, or negative otherwise.
int32_t pin_get_level(pin_num_t pin)
{
	//Gets the I/O pin bit from the IN or IN1 register
	if(pin < REG_BITS)
	{
		return REG_GET_BIT(GPIO_IN_REG, pin);
	}
	else
	{
		return REG_GET_BIT(GPIO_IN1_REG, pin-REG_BITS);
	}
}

// Get the value of the input registers, one pin per bit.
// The two 32-bit input registers are concatenated into a uint64_t.
uint64_t pin_get_in_reg(void)
{
	// Reada the IN and IN1 registers, returns the concatenated values
	uint64_t in0 = REG(GPIO_IN_REG);
	uint64_t in1 = REG(GPIO_IN1_REG);
	return in0 | (in1 << REG_BITS);
}

// Get the value of the output registers, one pin per bit.
// The two 32-bit output registers are concatenated into a uint64_t.
uint64_t pin_get_out_reg(void)
{
	//Reads the OUT and OUT1 registers, returns the concatenated values
	uint64_t in0 = REG(GPIO_OUT_REG);
	uint64_t in1 = REG(GPIO_OUT1_REG);
	return in0 | (in1 << REG_BITS);
}
