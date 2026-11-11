#include <stdbool.h>
#include <stdint.h>

#include "game.h"
#include "board.h"
#include "graphics.h"
#include "nav.h"
#include "config.h"
#include "hw.h"
#include "pin.h"
#include "com.h"

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

//initilize the game state and variables
void game_init(void)
{
    state = init_st;
    current_player = X_m;
    previous_a = false;
    previous_start = false;
}

// reads button pressed and updates state machine once per tick
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
        uint8_t stray;
        // Discard leftover data from the previous game.
        while (com_read(&stray, sizeof(stray)) > 0) {
            
        }

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
            {
                uint8_t location;
                bool have_move = false;

                if (a_pressed) {
                    nav_get_loc(&mark_row, &mark_col);

                    location = (uint8_t)(
                        ((uint8_t)mark_row << 4) |
                        ((uint8_t)mark_col & 0x0F)
                    );

                    // Send before checking whether the square is occupied.
                    com_write(&location, sizeof(location));
                    have_move = true;
                } else if (com_read(&location, sizeof(location)) == 1) {
                    mark_row = (location >> 4) & 0x0F;
                    mark_col = location & 0x0F;

                    have_move = true;
                }

                // Validate either a local move or a received move.
                if (have_move &&
                    mark_row >= 0 && mark_row < CONFIG_BOARD_R &&
                    mark_col >= 0 && mark_col < CONFIG_BOARD_C &&
                    board_set(mark_row, mark_col, current_player)) {
                    state = mark_st;
                }

                break;
            }

    case mark_st:
        // Draw the move, check if anyone won or if its full
        if (current_player == X_m)
        {
            graphics_drawX(mark_row, mark_col, CONFIG_MARK_CLR);
        }
        else
        {
            graphics_drawO(mark_row, mark_col, CONFIG_MARK_CLR);
        }
        //if someone one, display it in the corner of the screen
        if (board_winner(current_player)) 
        {
            if (current_player == X_m) {
                graphics_drawMessage("X wins", CONFIG_MESS_CLR, CONFIG_BACK_CLR);
            } else {
                graphics_drawMessage("O wins", CONFIG_MESS_CLR, CONFIG_BACK_CLR);
            }
            state = wait_restart_st;
        } 
        else if (board_mark_count() == CONFIG_BOARD_SPACES) 
        {
            graphics_drawMessage("DRAW", CONFIG_MESS_CLR, CONFIG_BACK_CLR);
            state = wait_restart_st;
        } 
        //switch between X and O turns
        else 
        {
            if (current_player == X_m) {
                current_player = O_m;
                graphics_drawMessage("O plays next", CONFIG_MESS_CLR, CONFIG_BACK_CLR);
            } 
            else
            {
                current_player = X_m;
                graphics_drawMessage("X plays next", CONFIG_MESS_CLR, CONFIG_BACK_CLR);
            }
            state = wait_mark_st;
        }
        break;

    case wait_restart_st:
        // Start a new game when START is pressed after a win or draw.
        if (start_pressed)
        {
            state = new_game_st;
        }
        break;
    }
}
