#ifndef PLANE_H_
#define PLANE_H_

#include "missile.h"

/******************** Plane Init Function ********************/

// Initialize the plane state machine. Pass a pointer to the missile
// that will be (re)launched by the plane. It will only have one missile.
void plane_init(missile_t *plane_missile);

/******************** Plane Control & Tick Functions ********************/

// Trigger the plane to explode.
void plane_explode(void);

// State machine tick function.
void plane_tick(void);

/******************** Plane Status Function ********************/

// Return the current plane position through the pointers *x,*y.
void plane_get_pos(coord_t *x, coord_t *y);

// Return whether the plane is flying.
bool plane_is_flying(void);

#endif // PLANE_H_
