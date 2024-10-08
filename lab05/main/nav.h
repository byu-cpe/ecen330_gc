#ifndef NAV_H_
#define NAV_H_

#include <stdint.h>

// This component depends on the joystick component to provide input for
// calculating the location of the navigator. If the joystick displacement
// is left, the navigator is moved incrementally left each time step
// through a call to nav_tick(). If the joystick is moved in another
// direction, the navigator is incrementally moved in that direction.
// The navigator location is clipped to the minimum and maximum dimensions.


// Initialize the navigator. Must be called before use.
// The initial location is the center of the grid.
// per: Specify the period in milliseconds that the navigator
// location is updated with a call to nav_tick().
// Return zero if successful, or non-zero otherwise.
int32_t nav_init(uint32_t per);

// Update the navigator location based on the joystick displacement.
// This function is not safe to call from an ISR context.
// Therefore, it must be called from a software task context.
void nav_tick(void);

// Set the sensitivity (speed) of the navigator relative to joystick movement.
// The sensitivity is specified as a factor in units of grid widths/sec
// at full joystick displacement.
// sens: joystick movement sensitivity in grid widths/sec.
void nav_set_sensitivity(float sens);

// Set the threshold of joystick displacement needed before moving the navigator.
// The threshold is specified as a factor (0 to 1) of maximum displacement.
// If this value is too low, the navigator will drift when the joystick is untouched.
// The default is a displacement factor of 0.075 from center location.
// thr: threshold factor (0 to 1)
void nav_set_threshold(float thr);

// Get the navigator location.
// Values range from 0 to grid maximum width and height minus 1.
// *r: pointer to row grid location.
// *c: pointer to column grid location.
void nav_get_loc(int8_t *r, int8_t *c);

// Set the navigator location.
// Values range from 0 to grid maximum width and height minus 1.
// r: row grid location.
// c: column grid location.
void nav_set_loc(int8_t r, int8_t c);

#endif // NAV_H_
