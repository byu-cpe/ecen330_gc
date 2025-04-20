
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

// All missiles
missile_t missiles[CONFIG_MAX_TOTAL_MISSILES];

// Alias into missiles array
missile_t *enemy_missiles = missiles+0;
missile_t *player_missiles = missiles+CONFIG_MAX_ENEMY_MISSILES;
missile_t *plane_missile = missiles+CONFIG_MAX_ENEMY_MISSILES+
									CONFIG_MAX_PLAYER_MISSILES;

// M2: Declare stats variables


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

	// M2: Set sound volume
}

// Update the game control logic.
// This function calls the missile & plane tick functions, relaunches
// idle enemy missiles, handles button presses, launches player missiles,
// detects collisions, and updates statistics.
void game_tick(void)
{
	// Tick missiles in one batch
	for (uint32_t i = 0; i < CONFIG_MAX_TOTAL_MISSILES; i++)
		missile_tick(missiles+i);

	// Tick plane
	plane_tick();

	// Relaunch idle enemy missiles
	for (uint32_t i = 0; i < CONFIG_MAX_ENEMY_MISSILES; i++)
		if (missile_is_idle(enemy_missiles+i))
			missile_launch_enemy(enemy_missiles+i);

	// M1: Relaunch idle player missiles, !!! remove after Milestone 1 !!!
	for (uint32_t i = 0; i < CONFIG_MAX_PLAYER_MISSILES; i++)
		if (missile_is_idle(player_missiles+i))
			missile_launch_player(player_missiles+i, rand()%LCD_W, rand()%LCD_H);

	// M2: Check for button press. If so, launch a free player missile.

	// M2: Check for moving non-player missile collision with an explosion.

	// M2: Check for flying plane collision with an explosion.

	// M2: Count non-player impacted missiles

	// M2: Draw stats
}
