
#include <stdlib.h> // abs
#include <stdbool.h>

#include "joy.h"
#include "nav.h"
#include "config.h"

#define GRID_C CONFIG_BOARD_C
#define GRID_R CONFIG_BOARD_R

// The sensitivity constant is in cells/sec and is converted to grids/sec
#define SEN_DEFAULT (3.0f/CONFIG_BOARD_C) // Grid widths per second
#define THRESH_DEFAULT 0.75f // Raw ADC values
#define CLIP(x,lo,hi) ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))

static uint32_t uperiod; // Update period in milliseconds.
static float sfactor; // Joystick sensitivity factor.
static uint32_t thresh; // Joystick displacement threshold.
static float rloc, cloc; // Current navigator location as a float.


// Initialize the navigator. Must be called before use.
// The initial location is the center of the grid.
// per: Specify the period in milliseconds that the navigator
// location is updated with a call to nav_tick().
// Return zero if successful, or non-zero otherwise.
int32_t nav_init(uint32_t per)
{
	if (per == 0 || joy_init()) return -1;
	uperiod = per; // Save period parameter
	// Set defaults
	nav_set_sensitivity(SEN_DEFAULT);
	nav_set_threshold(THRESH_DEFAULT);
	// Initialize the navigator location to the center of the grid.
	rloc = GRID_R/2;
	cloc = GRID_C/2;
	return 0;
}

// Update the navigator location based on the joystick displacement.
// This function is not safe to call from an ISR context.
// Therefore, it must be called from a software task context.
void nav_tick(void)
{
	static bool last_move = false;
	int32_t dcx, dcy;

	// Get joystick position relative to the center in raw ADC values.
	joy_get_displacement(&dcx, &dcy);
	if (abs(dcx) < thresh && abs(dcy) < thresh) {
		rloc = (int)(rloc+0.5f);
		cloc = (int)(cloc+0.5f);
		last_move = false;
		return;
	}

	// Based on the joystick position relative to center,
	// calculate a new location for the navigator.
	if (last_move) {
		rloc += dcy*sfactor;
		cloc += dcx*sfactor;
	} else {
		// Provides bump on first move.
		if (abs(dcy) >= thresh)
			rloc += (dcy < 0) ? -0.5f : +0.5f;
		if (abs(dcx) >= thresh)
			cloc += (dcx < 0) ? -0.5f : +0.5f;
	}

	// Clip new location to grid.
	rloc = CLIP(rloc, 0, GRID_R-1);
	cloc = CLIP(cloc, 0, GRID_C-1);

	last_move = true;
}

// Set the sensitivity (speed) of the navigator relative to joystick movement.
// The sensitivity is specified as a factor in units of grid widths/sec
// at full joystick displacement.
// sens: joystick movement sensitivity in grid widths/sec.
void nav_set_sensitivity(float sens)
{
	float rate = sens*GRID_C; // Convert to cells per second
	sfactor = rate / JOY_MAX_DISP * uperiod / 1000;
}

// Set the threshold of joystick displacement needed before moving the navigator.
// The threshold is specified as a factor (0 to 1) of maximum displacement.
// If this value is too low, the navigator will drift when the joystick is untouched.
// The default is a displacement factor of 0.075 from center location.
// thr: threshold factor (0 to 1)
void nav_set_threshold(float thr)
{
	thresh = thr*JOY_MAX_DISP; // Convert to raw ADC values
}

// Get the navigator location.
// Values range from 0 to grid maximum width and height minus 1.
// *r: pointer to row grid location.
// *c: pointer to column grid location.
void nav_get_loc(int8_t *r, int8_t *c)
{
	*r = rloc+0.5f;
	*c = cloc+0.5f;
}

// Set the navigator location.
// Values range from 0 to grid maximum width and height minus 1.
// r: row grid location.
// c: column grid location.
void nav_set_loc(int8_t r, int8_t c)
{
	// Clip new location to grid.
	rloc = CLIP(r, 0, GRID_R-1);
	cloc = CLIP(c, 0, GRID_C-1);
}
