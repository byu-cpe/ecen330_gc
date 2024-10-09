#ifndef GRAPHICS_H_
#define GRAPHICS_H_
/**
 * @file
 * @brief Functions to draw the grid and primitive shapes.
 * @details Row and column grid locations are numbered from 0 to N-1 starting
 * from the top left and increase by 1 moving to the right and down.
 */

#include <stdint.h>
#include "lcd.h"

/**
 * @brief Draw lines that form the playing grid.
 */
void graphics_drawGrid(color_t color);

/**
 * @brief Draw a string in the message area.
 * @param str   ASCII encoded string, zero terminated.
 * @param color Foreground (text) color value.
 * @param bg    Background (surrounding) color value.
 */
void graphics_drawMessage(const char *str, color_t color, color_t bg);

/**
 * @brief Draw an X at the specified location.
 * @param r     Row grid location.
 * @param c     Column grid location.
 * @param color Color value.
 */
void graphics_drawX(int8_t r, int8_t c, color_t color);

/**
 * @brief Draw an O at the specified location.
 * @param r     Row grid location.
 * @param c     Column grid location.
 * @param color Color value.
 */
void graphics_drawO(int8_t r, int8_t c, color_t color);

/**
 * @brief Draw a box to highlight the specified location.
 * @param r     Row grid location.
 * @param c     Column grid location.
 * @param color Color value.
 */
void graphics_drawHighlight(int8_t r, int8_t c, color_t color);

#endif // GRAPHICS_H_
