
#include <stdio.h>
#include <hw_gc.h>
#include <stdint.h>
#include <ball.h>


// M2: Define stats constants
#define LCD_W HW_LCD_W
#define LCD_H HW_LCD_W




// M2: Declare stats variables
static uint16_t player_score; 
static uint16_t enemy_score;
static char player_score_buffer[16];
static char enemy_score_buffer[16];

// other definitions
const uint32_t VOLUME = 100;

// Initialize the game control logic.
// This function initializes all missiles, planes, stats, etc.
void game_init(void)
{
	// Initialize user paddle
	user_paddle_init();

	// Initialize ball
	ball_init();

    //init enemy paddle
    enemy_paddle_init();

	// M2: Initialize stats
	player_score = 0;
	enemy_score = 0;
	sprintf(player_score_buffer, "Player: %d", player_score);
	sprintf(enemy_score_buffer, "Oponent: %d", enemy_score);
	lcd_drawString(LCD_W,0, enemy_score_buffer, WHITE);
    lcd_drawString(LCD_W,LCD_H, player_score_buffer, WHITE);

	// M2: Set sound volume
	sound_set_volume(VOLUME);
}

// Update the game control logic.
// This function calls the missile & plane tick functions, relaunches
// idle enemy missiles, handles button presses, launches player missiles,
// detects collisions, and updates statistics.
void game_tick(void)
{
    ball_tick();
    enemy_paddle_tick();
    user_paddle_tick();

	
	//check for start button pressed, if so then restart



	// M2: Check for a ball and paddle collision
	for (uint32_t i = 0; i < CONFIG_MAX_TOTAL_MISSILES; i++)
	{
		//check
	}

    //check for ball out of bounds
    for (uint32_t i = 0; i < CONFIG_MAX_TOTAL_MISSILES; i++){
		
	}

	// udpate score
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
