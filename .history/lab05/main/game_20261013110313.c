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
    // Read buttons and detect new presses here.

    switch (state) {
    case init_st:
        state = new_game_st;
        break;

    case new_game_st:
        // Reset and draw the game.
        break;

    case wait_mark_st:
        // Accept a valid move.
        break;

    case mark_st:
        // Draw the move and determine the result.
        break;

    case wait_restart_st:
        // Accept a restart.
        break;
    }
}