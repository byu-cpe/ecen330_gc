#include "missile.h"
#include "config.h"
#include "lcd.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "plane.h"
#include "game.h"
#include <hw.h>
#include <cursor.h>
#include <sound.h>
#include <pin.h>



#define SILO_1      (LCD_W / 4)
#define SILO_2      (LCD_W * 2 / 4)
#define SILO_3      (LCD_W * 3 / 4)
#define PARTITION_1 (LCD_W * 3 / 8)
#define PARTITION_2 (LCD_W * 5 / 8)

//check my float types to see if thats what the coding standard wants


typedef enum{
    Idle,
    Moving,
    Exploding_Growing,
    Exploding_Shrinking,
    Impacted
}game_state;





/******************** Missile Init & Launch Functions ********************/

// Different _launch_ functions are used depending on the missile type.

// Initialize the missile as an idle missile. When initialized to the idle
// state, a missile doesn't appear nor does it move. The launch flag should
// also be set to false. Other missile_t members will be set up at launch.
void missile_init(missile_t *missile)
{
    missile->currentState = Idle;
    missile->launch = false;
}

// Launch the missile as a player missile. This function takes an (x, y)
// destination of the missile (as specified by the user). The origin is the
// closest "firing location" to the destination (there are three firing
// locations evenly spaced along the bottom of the screen).
void missile_launch_player(missile_t *missile, coord_t x_dest, coord_t y_dest)
{
    missile->type = MISSILE_TYPE_PLAYER;
    missile->launch = true;
    missile->explode_me = false;
    missile->length = 0;
    missile->currentState = Idle;
    missile->x_dest = x_dest;
    missile->y_dest = y_dest;
    missile->y_origin = LCD_H-1;
    if (x_dest <= PARTITION_1)
    {
        missile->x_origin = SILO_1;
    }
    else if (x_dest >= PARTITION_1 && x_dest <= PARTITION_2)
    {
        missile->x_origin = SILO_2;
    }
    else if (x_dest > PARTITION_2)
    {
        missile->x_origin = SILO_3;
    }
    float y_dif = (missile->y_dest - missile->y_origin);
    float x_dif = (missile->x_dest - missile->x_origin);
    missile->total_length = sqrtf((y_dif * y_dif) + (x_dif * x_dif));
    return;
}

// Launch the missile as an enemy missile. This will randomly choose the
// origin and destination of the missile. The origin is somewhere near the
// top of the screen, and the destination is the very bottom of the screen.
void missile_launch_enemy(missile_t *missile)
{
    missile->type = MISSILE_TYPE_ENEMY;
    missile->launch = true;
    missile->explode_me = false;
    missile->length = 0;
    missile->x_dest = rand() % LCD_W;
    missile->y_dest = LCD_H-1;
    missile->x_origin = rand() % LCD_W;
    missile->y_origin = rand() % (LCD_H /4);
    float y_dif = (missile->y_dest - missile->y_origin);
    float x_dif = (missile->x_dest - missile->x_origin);
    missile->total_length = sqrtf((y_dif * y_dif) + (x_dif * x_dif));
    return;
}

// Launch the missile as a plane missile. This function takes the (x, y)
// location of the plane as an argument and uses it as the missile origin.
// The destination is randomly chosen along the bottom of the screen.
void missile_launch_plane(missile_t *missile, coord_t x_orig, coord_t y_orig)
{
    missile->type = MISSILE_TYPE_PLANE;
    missile->launch = true;
    missile->explode_me = false;
    missile->length = 0;
    missile->x_origin = x_orig;
    missile->y_origin = y_orig;
    missile->x_dest = rand() % LCD_W;
    missile->y_dest = LCD_H-1;
    float y_dif = (missile->y_dest - missile->y_origin);
    float x_dif = (missile->x_dest - missile->x_origin);
    missile->total_length = sqrtf((y_dif * y_dif) + (x_dif * x_dif));

}

/******************** Missile Control & Tick Functions ********************/

// Used to indicate that a moving missile should be detonated. This occurs
// when an enemy or a plane missile is located within an explosion zone.
void missile_explode(missile_t *missile)
{
    //missile->currentState = Exploding_Growing;
    missile->explode_me = true;
}



/******************** Missile Status Functions ********************/

// Return the current missile position through the pointers *x,*y.
void missile_get_pos(missile_t *missile, coord_t *x, coord_t *y)
{
    *x = (missile->x_current);
    *y = (missile->y_current);
    return;
}

// Return the missile type.
missile_type_t missile_get_type(missile_t *missile)
{
    return missile->type;
}

// Return whether the given missile is moving.
bool missile_is_moving(missile_t *missile)
{
    return (missile->currentState == Moving);
}

// Return whether the given missile is exploding. If this missile
// is exploding, it can explode another intersecting missile.
bool missile_is_exploding(missile_t *missile)
{
    return (missile->radius > 0);
}

// Return whether the given missile is idle.
bool missile_is_idle(missile_t *missile)
{
    return (missile->currentState == Idle);
}

// Return whether the given missile is impacted.
bool missile_is_impacted(missile_t *missile)
{
    return (missile->currentState == Impacted);
}

// Return whether an object (e.g., missile or plane) at the specified
// (x,y) position is colliding with the given missile. For a collision
// to occur, the missile needs to be exploding and the specified
// position needs to be within the explosion radius.
bool missile_is_colliding(missile_t *missile, coord_t x, coord_t y)
{
    if (!missile_is_exploding(missile)) return false;

    float y_dif = (missile->x_current - x);
    float x_dif = (missile->y_current - y);

    return (((y_dif * y_dif) + (x_dif * x_dif)) <= missile->radius * missile->radius);
}


// Tick the state machine for a single missile.
void missile_tick(missile_t *missile){


    //switch state for transitions
    switch(missile->currentState){
        case Idle:
            printf("idle   ");
            if (missile->launch == true)
            {
                missile->currentState = Moving;
                missile->launch = false; //idk if this is right or should go in the actions
            }
            break;
        case Moving:
            printf("moving state");
            if 
            if (missile->length >= missile->total_length)
            {   
                missile->currentState = Impacted;
            }
            break;
        case Exploding_Growing:
        printf("exploding growing state");
            if (missile->radius <= CONFIG_EXPLOSION_MAX_RADIUS)
            {
                missile->currentState = Exploding_Shrinking;
            }
            break;
        case Exploding_Shrinking:
        printf("exploding shrinking");
            if (missile->radius <= 0)
            {
                missile->currentState = Idle;
            }
            break;
        case Impacted:
        printf("impacted");
            missile->currentState = Idle;
            break;
    }

    //switch state for actions
    switch(missile->currentState){
        case Idle:
            //reset something idk?
            break;
        case Moving:

            float fraction = missile->length / missile->total_length;
            missile->x_current = missile->x_origin + fraction * (missile->x_dest - missile->x_origin);
            missile->y_current = missile->y_origin + fraction * (missile->y_dest - missile->y_origin);
            if (missile->type == MISSILE_TYPE_PLAYER)
            {
                missile->x_current = missile->x_current + CONFIG_PLAYER_MISSILE_DISTANCE_PER_TICK;
                missile->y_current = missile->y_current + CONFIG_PLAYER_MISSILE_DISTANCE_PER_TICK;

                lcd_drawLine(missile->x_origin, missile->y_origin, missile->x_current, missile->y_current,CONFIG_COLOR_PLAYER_MISSILE);
            }
            else 
            {
                missile->x_current = missile->x_current + CONFIG_ENEMY_MISSILE_DISTANCE_PER_TICK;
                missile->y_current = missile->y_current + CONFIG_ENEMY_MISSILE_DISTANCE_PER_TICK;
                lcd_drawLine(missile->x_origin, missile->y_origin, missile->x_current, missile->y_current,CONFIG_COLOR_ENEMY_MISSILE);
            } 
            
            break;
        case Exploding_Growing:
            missile->radius += CONFIG_EXPLOSION_RADIUS_CHANGE_PER_TICK;
            if(missile->type == MISSILE_TYPE_PLAYER)
            {
            lcd_fillCircle(missile->x_current,missile->y_current, missile->radius, CONFIG_COLOR_PLAYER_MISSILE);
            }
            else
            {
            lcd_fillCircle(missile->x_current,missile->y_current, missile->radius, CONFIG_COLOR_ENEMY_MISSILE);
            }
            break;
        case Exploding_Shrinking:
            missile->radius -= CONFIG_EXPLOSION_RADIUS_CHANGE_PER_TICK;
            if(missile->type == MISSILE_TYPE_PLAYER)
            {
            lcd_fillCircle(missile->x_current,missile->y_current, missile->radius, CONFIG_COLOR_PLAYER_MISSILE);
            }
            else
            {
            lcd_fillCircle(missile->x_current,missile->y_current, missile->radius, CONFIG_COLOR_ENEMY_MISSILE);
            }
            break;
        case Impacted:
            break;
    }
}

