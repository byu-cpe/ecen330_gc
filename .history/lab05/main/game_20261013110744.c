#include <stdbool.h>
#include <stdint.h>

#include "game.h"
#include "board.h"
#include "graphics.h"
#include "nav.h"
#include "config.h"
#include "hw.h"
#include "pin.h"

typedef enum {
    init_st,
    new_game_st,
    wait_mark_st,
    mark_st,
    wait_restart_st
} game_state_t;

static game_state_t state;
static mark_t current_player;
static int8_t mark_row, mark_col;

static bool previous_a;
static bool previous_start;

void game_init(void)
{
    state = init_st;
    current_player = X_m;
    previous_a = false;
    previous_start = false;
}

void game_tick(void)
{
    // Read buttons and detect new presses here, buttons all use pullups so 0 means pressed
    bool a_down = (pin_get_level(HW_BTN_A) == 0);
    bool start_down = (pin_get_level(HW_BTN_START) == 0);

    bool a_pressed = a_down && !previous_a;
    bool start_pressed = start_down && !previous_start;

    previous_a = a_down;
    previous_start = start_down;


    switch (state) {
    case init_st:
        state = new_game_st;
        break;

    case new_game_st:
        // Reset and draw the game, clears the board and its data too
        lcd_fillScreen(CONFIG_BACK_CLR);
        graphics_drawGrid(CONFIG_GRID_CLR);
        board_clear();

        current_player = X_m;

        nav_set_loc(CONFIG_BOARD_R / 2, CONFIG_BOARD_C / 2);

        graphics_drawMessage("X plays next",
                            CONFIG_MESS_CLR, CONFIG_BACK_CLR);

        state = wait_mark_st;
        break;

    case wait_mark_st:
        // Accept a valid move otherwise 
        if (a_pressed) {
            nav_get_loc(&mark_row, &mark_col);

            if (board_set(mark_row, mark_col, current_player)) {
                state = mark_st;
            }
        }
        break;

    case mark_st:
        // Draw the move and determine the result.
        break;

    case wait_restart_st:
        // Accept a restart.
        break;
    }
}