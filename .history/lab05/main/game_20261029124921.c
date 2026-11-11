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

static bool have_move = false;

const int8_t BITMASK4 =4;
const int8_t BITMASK16 =0x0F;


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
    
    //shouldnt be here
    // bool a_pressed = (!pin_get_level(HW_BTN_A));
    //         bool start_pressed = (!pin_get_level(HW_BTN_START));

    //transitions
    switch (state) {
    case init_st:
        state = new_game_st;
        break;
    //draws the initial message and moves to wait mark state
    case new_game_st:
        {

        graphics_drawMessage("X plays next", CONFIG_MESS_CLR, CONFIG_BACK_CLR);
        state = wait_mark_st;
        break;
        }
    
    //if the move it valid, then mark it by moving to mark state
    case wait_mark_st:
        {
            uint8_t location;
            bool have_move = false;
            // Validate either a local move or a received move.
            if (a_pressed) 
            {
            nav_get_loc(&mark_row, &mark_col);

            location = (uint8_t)(
                ((uint8_t)mark_row << 4) |
                ((uint8_t)mark_col & 0x0F)
            );

                //Send before checking whether the square is occupied.
                com_write(&location, sizeof(location));
                have_move = true;
            } 
            if (have_move &&
                mark_row >= 0 && mark_row < CONFIG_BOARD_R &&
                mark_col >= 0 && mark_col < CONFIG_BOARD_C &&
                board_set(mark_row, mark_col, current_player))
                {
                    state = mark_st;
                }
            }
          
            break;
        }
    //check if there was a win and move to wait restart state, if not then move back to wait mark state
    case mark_st:
    
        //determines the winner
        if (board_winner(current_player)) 
        {   
            //displays which player won the game
            if (current_player == X_m) 
            {
                graphics_drawMessage("X wins", CONFIG_MESS_CLR, CONFIG_BACK_CLR);
            } 
            else 
            {
                graphics_drawMessage("O wins", CONFIG_MESS_CLR, CONFIG_BACK_CLR);
            }
            state = wait_restart_st;
        } 
        //if it was a draw then wait for a restart and display that it was a draw
        else if (board_mark_count() == CONFIG_BOARD_SPACES) 
        {
            graphics_drawMessage("DRAW", CONFIG_MESS_CLR, CONFIG_BACK_CLR);
            state = wait_restart_st;
        } 
        //switch between X and O turns
        else 
        {
            //switches between X and O for turns, transitions to wait mark state
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
    
    //if the start button is pressed, move to the new game state
    case wait_restart_st:
        // Start a new game when START is pressed after a win or draw.
        if (!pin_get_level(HW_BTN_START))
        {
            state = new_game_st;
        }
        break;
    }

    //This is the switch state that contains the actions of the state machine
    switch(state)
    {
    //nothing happens during the initialization here
    case init_st:
        break;
    //when the state shifts to a new game, it clears the screen and sets everything up for a new game
    case new_game_st:
        {
        // Reset and draw the game, clears the board and its data too
        uint8_t stray;
        lcd_fillScreen(CONFIG_BACK_CLR);
        graphics_drawGrid(CONFIG_GRID_CLR);
        board_clear();
        current_player = X_m;
        nav_set_loc(CONFIG_BOARD_R / 2, CONFIG_BOARD_C / 2);
        while (com_read(&stray,1)){}
        break;
        }
    //
    case wait_mark_st:
        {
        uint8_t location;
        //when button A is pressed it gets the location of where the cursor is at and checks if it can mark that space and if it can then it marks it
        if (!pin_get_level(HW_BTN_A)) 
        {
            nav_get_loc(&mark_row, &mark_col);

            location = (uint8_t)(
                ((uint8_t)mark_row << BITMASK4) |
                ((uint8_t)mark_col & BITMASK16)
            );

            //Send before checking whether the square is occupied.
            com_write(&location, sizeof(location));
            have_move = true;
        } 
        else if (com_read(&location, sizeof(location)) == 1) 
        {
            mark_row = (location >> BITMASK4);
            mark_col = location & BITMASK16;

            have_move = true;
        }
        break;
        }
    //marks the selected spot
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
        break;
    //nothing happens here
    case wait_restart_st:
        break;
    }
}



