#include "missile.h"
#include "config.h"
#include <math.h>
#include <stdio.h>

typedef enum{
    Idle,
    Flying
} plane_state;

static missile_t *global_plane_missle;
static plane_state state;
static uint16_t plane_idle_tick_counter = 0;
static uint16_t x_pos = 0;
const uint16_t y_pos = (LCD_H / 4 );
bool alread_launched_missile;

/******************** Plane Init Function ********************/

// Initialize the plane state machine. Pass a pointer to the missile
// that will be (re)launched by the plane. It will only have one missile.
void plane_init(missile_t *plane_missile)
{
    global_plane_missle = plane_missile;
    plane_missile->currentState = Idle;
    alread_launched_missile = false;
    state = Idle;
}

/******************** Plane Control & Tick Functions ********************/

// Trigger the plane to explode.
void plane_explode(void)
{
    state = Idle;
    x_pos = 0;
    plane_idle_tick_counter = 0;

}


/******************** Plane Status Functions ********************/

// Return the current plane position through the pointers *x,*y.
void plane_get_pos(coord_t *x, coord_t *y)
{
    *x = x_pos;
    *y = y_pos;
    return;
}

// Return whether the plane is flying.
bool plane_is_flying(void)
{
    return (state == Flying);
}

// State machine tick function.
void plane_tick(void) 
{    
    //switch for transitions
    switch(state){
        case Idle:
            if (plane_idle_tick_counter >= CONFIG_PLANE_IDLE_TIME_TICKS)
            {
                state = Flying;
                x_pos = LCD_W;
            }
            break;

        case Flying:
            if (x_pos <= 0)
            {
                state = Idle;
                plane_idle_tick_counter = 0;
            }
        
            break;
    }
    //switch for actions
    switch(state)
    {
        case Idle:
            plane_idle_tick_counter +=1;
            break;
        case Flying:
            //if (x_pos >= (LCD_W / 2) )
            printf("reached");
            if(!alread_launched_missile && (x_pos <= (LCD_W / 2)) && missile_is_idle(global_plane_missle))
            lcd_fillTriangle(x_pos, y_pos, x_pos + CONFIG_PLANE_WIDTH, y_pos - (CONFIG_PLANE_HEIGHT/2), x_pos + CONFIG_PLANE_WIDTH, y_pos + (CONFIG_PLANE_HEIGHT/2), CONFIG_COLOR_PLANE);
            x_pos -= CONFIG_PLANE_DISTANCE_PER_TICK;
            break;
    }
}

