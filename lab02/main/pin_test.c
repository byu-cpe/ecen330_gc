#include <stdint.h>
#include "soc/soc.h" // REG_READ
#include "soc/gpio_reg.h" // GPIO_PIN0_REG, GPIO_FUNC0_OUT_SEL_CFG_REG
#include "soc/gpio_periph.h" // GPIO_PIN_MUX_REG
#include "pin_test.h"

// Get the value of the PIN register for the specified pin argument.
uint32_t pin_test_get_pin_reg(pin_num_t pin)
{
	return REG_READ(GPIO_PIN0_REG + (pin * sizeof(uint32_t)));
}

// Get the value of the output function register for the specified pin argument.
uint32_t pin_test_get_func_out_sel_cfg_reg(pin_num_t pin)
{
	return REG_READ(GPIO_FUNC0_OUT_SEL_CFG_REG + (pin * sizeof(uint32_t)));
}

// Get the value of the IO MUX register for the specified pin argument.
uint32_t pin_test_get_io_mux_reg(pin_num_t pin)
{
	return REG_READ(GPIO_PIN_MUX_REG[pin]);
}

/*
For details see:
/opt/esp6/esp-idf/components/soc/include/soc/gpio_periph.h
/opt/esp6/esp-idf/components/soc/esp32/include/soc/soc.h
/opt/esp6/esp-idf/components/soc/esp32/register/soc/gpio_reg.h
/opt/esp6/esp-idf/components/soc/esp32/register/soc/reg_base.h
*/
