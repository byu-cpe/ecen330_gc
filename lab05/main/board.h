#ifndef BOARD_H_
#define BOARD_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	no_m,
	X_m,
	O_m,
} mark_t;

// Clear the board
void board_clear(void);

// Get mark at board location
mark_t board_get(int8_t r, int8_t c);

// If location empty, set the mark and return true, otherwise return false.
bool board_set(int8_t r, int8_t c, mark_t mark);

// Check if mark type is a winner.
bool board_winner(mark_t mark);

// Get a count of marks in the board.
// Use to determine a draw condition if count is equal to CONFIG_BOARD_SPACES
uint16_t board_mark_count(void);

#endif // BOARD_H_
