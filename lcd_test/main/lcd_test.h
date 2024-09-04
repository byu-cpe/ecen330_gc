#ifndef LCD_TEST_H_
#define LCD_TEST_H_
/**
 * @file
 * @brief Functions to test the LCD display component.
 */

#include <stdint.h>

/**
 * @brief Calls all the tests in a forever loop.
 * @param pvParameters Not used.
 */
void lcd_test_all(void *pvParameters);

#endif // LCD_TEST_H_
