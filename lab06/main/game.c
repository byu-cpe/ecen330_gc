
#include <stdio.h>
#include <stdlib.h> // rand

#include "hw.h"
#include "lcd.h"
#include "cursor.h"
#include "sound.h"
#include "pin.h"
#include "missile.h"
#include "plane.h"
#include "game.h"
#include "config.h"

// sound support
#include "missileLaunch.h"

// M2: Define stats constants
#define SHOTS_STRING_X (LCD_W / 4)
#define SHOTS_STRING_Y 10
#define IMPACTED_STRING_X (LCD_W / 2)
#define IMPACTED_STRING_Y 10


// All missiles
missile_t missiles[CONFIG_MAX_TOTAL_MISSILES];

// Alias into missiles array
missile_t *enemy_missiles = missiles+0;
missile_t *player_missiles = missiles+CONFIG_MAX_ENEMY_MISSILES;
missile_t *plane_missile = missiles+CONFIG_MAX_ENEMY_MISSILES+
									CONFIG_MAX_PLAYER_MISSILES;

// M2: Declare stats variables
static uint16_t shots; 
static uint16_t impacted;
static char shots_buffer[16];
static char impacted_buffer[16];

// other definitions
const uint32_t VOLUME = 100;

// Initialize the game control logic.
// This function initializes all missiles, planes, stats, etc.
void game_init(void)
{
	// Initialize missiles
	for (uint32_t i = 0; i < CONFIG_MAX_TOTAL_MISSILES; i++)
		missile_init(missiles+i);

	// Initialize plane
	plane_init(plane_missile);

	// M2: Initialize stats
	shots = 0;
	impacted = 0;
	sprintf(shots_buffer, "shot: %d", shots);
	sprintf(impacted_buffer, "impacted: %d", impacted);
	lcd_drawString(SHOTS_STRING_X, SHOTS_STRING_Y, shots_buffer, CONFIG_COLOR_STATUS);
	lcd_drawString(IMPACTED_STRING_X, IMPACTED_STRING_Y, impacted_buffer, CONFIG_COLOR_STATUS);

	// M2: Set sound volume
	sound_set_volume(VOLUME);
}

// Update the game control logic.
// This function calls the missile & plane tick functions, relaunches
// idle enemy missiles, handles button presses, launches player missiles,
// detects collisions, and updates statistics.
void game_tick(void)
{
	// Tick missiles in one batch
	for (uint32_t i = 0; i < CONFIG_MAX_TOTAL_MISSILES; i++){
		missile_tick(missiles+i);
	}

	// Tick plane
	plane_tick(); // Move plane and launch plane missile

	// Relaunch idle enemy missiles
	for (uint32_t i = 0; i < CONFIG_MAX_ENEMY_MISSILES; i++)
		//launch enemy missile if its idle
		if (missile_is_idle(enemy_missiles+i))
			missile_launch_enemy(enemy_missiles+i);

	// M2: Check for button press. If so, launch a free player missile.
	static bool pressed = false;
	coord_t x, y;
	uint64_t btns;
	btns = ~pin_get_in_reg() & HW_BTN_MASK;
	//if button is pressed, loop missiles and launch it if its idle
	if (!pressed && btns) {
		pressed = true; // button pressed
		cursor_get_pos(&x, &y);
		// loop through all player missiles
		for (uint32_t i = 0; i < CONFIG_MAX_PLAYER_MISSILES; i++)
		{
			//if a player missile is idle launch it to the target (x,y) position.
			if (missile_is_idle(player_missiles+i))
			{
				shots++;
				missile_launch_player(player_missiles+i, x, y);
				sound_start(missileLaunch, sizeof(missileLaunch),false);
				break;
			}
		}
	}
	//or if the button isnt pressed then release the pressed variabel to false
	else if (pressed && !btns) 
	{
	pressed = false; // all released
	}


	// M2: Check for moving non-player missile collision with an explosion.
	for (uint32_t i = 0; i < CONFIG_MAX_TOTAL_MISSILES; i++)
	{
		//if its a player missile, then skip the collison check
		if((missiles+i)->type == MISSILE_TYPE_PLAYER)
		{
			continue;
		}
		//if its not moving, then skip also
		if(!missile_is_moving(missiles+i))
		{
			continue;
		}

		
		coord_t missile_x, missile_y;
		missile_get_pos(missiles+i, &missile_x, &missile_y);
		//loop through the rest of the misiles to see if any are colliding
		for (uint32_t j = 0; j < CONFIG_MAX_TOTAL_MISSILES; j++)
		{	
			//check if any missiles are colliding, if they are explode the enemy missile
			if(missile_is_colliding(missiles+j, missile_x, missile_y))
			{
				missile_explode(missiles+i);
				break;
			}
				
			
			
		}
	}

	// M2: Check for flying plane collision with an explosion.
	for (uint32_t i = 0; i < CONFIG_MAX_TOTAL_MISSILES; i++){
		//if the missiles is exploding, proceed to get the coords
		if (missile_is_exploding(missiles+i))
		{
			coord_t plane_x, plane_y;
			plane_get_pos(&plane_x, &plane_y);
			//if the missile is colliding with the plane, explode the plane
			if (missile_is_colliding(missiles+i, plane_x, plane_y))
			{
				plane_explode();
				break;
			}
		}
	}

	// M2: Count non-player impacted missiles
	for (uint32_t i = 0; i < CONFIG_MAX_ENEMY_MISSILES; i++)
		//if the missile is impacted, increase the impacted counter at the top of the screen
		if (missile_is_impacted(enemy_missiles+i)){
			impacted++;
		}

	// M2: Draw stats
	sprintf(shots_buffer, "shot: %d", shots);
	sprintf(impacted_buffer, "impacted: %d", impacted);
	lcd_drawString(SHOTS_STRING_X, SHOTS_STRING_Y, shots_buffer, CONFIG_COLOR_STATUS);
	lcd_drawString(IMPACTED_STRING_X, IMPACTED_STRING_Y, impacted_buffer, CONFIG_COLOR_STATUS);
}
