#include "board.h"
#include "config.h"

#define BOARD_R CONFIG_BOARD_R // Rows
#define BOARD_C CONFIG_BOARD_C // Columns
#define BOARD_N CONFIG_BOARD_N // Number of contiguous marks
#define BOARD_SPACES CONFIG_BOARD_SPACES

static mark_t board[BOARD_R][BOARD_C];
static uint16_t mark_count;


// Clear the board
void board_clear(void)
{
	for (int8_t r = 0; r < BOARD_R; r++)
		for (int8_t c = 0; c < BOARD_C; c++)
			board[r][c] = no_m;
	mark_count = 0;
}

// Get mark at board location
mark_t board_get(int8_t r, int8_t c)
{
	return board[r][c];
}

// If location empty, set the mark and return true, otherwise return false.
bool board_set(int8_t r, int8_t c, mark_t mark)
{
	if (board[r][c] == no_m) {
		board[r][c] = mark;
		mark_count++;
		return true;
	} else return false;
}

// Check if mark type is a winner.
bool board_winner(mark_t mark)
{
	int16_t r; // Row
	int16_t c; // Column
	int16_t n; // Contiguous marks
	int16_t d; // Diagonal (can be row or column)

	// Horizontal (-) cases
	for (r = 0; r < BOARD_R; r++)
		for (c = 0, n = 0; c < BOARD_C; c++)
			if (board[r][c] == mark) {
				if (++n == BOARD_N) return true;
			} else n = 0;
	// Vertical (|) cases
	for (c = 0; c < BOARD_C; c++)
		for (r = 0, n = 0; r < BOARD_R; r++)
			if (board[r][c] == mark) {
				if (++n == BOARD_N) return true;
			} else n = 0;
	// Diagonal up (/) cases
	for (d = 0; d < BOARD_R; d++)
		for (r = d, c = 0, n = 0; r >= 0 && c < BOARD_C; r--, c++)
			if (board[r][c] == mark) {
				if (++n == BOARD_N) return true;
			} else n = 0;
	for (d = 1; d < BOARD_C; d++)
		for (r = BOARD_R-1, c = d, n = 0; r >= 0 && c < BOARD_C; r--, c++)
			if (board[r][c] == mark) {
				if (++n == BOARD_N) return true;
			} else n = 0;
	// Diagonal down (\) cases
	for (d = 0; d < BOARD_R; d++)
		for (r = d, c = BOARD_C-1, n = 0; r >= 0 && c >= 0; r--, c--)
			if (board[r][c] == mark) {
				if (++n == BOARD_N) return true;
			} else n = 0;
	for (d = BOARD_C-2; d >= 0; d--)
		for (r = BOARD_R-1, c = d, n = 0; r >= 0 && c >= 0; r--, c--)
			if (board[r][c] == mark) {
				if (++n == BOARD_N) return true;
			} else n = 0;
	return false;
}

// Get a count of marks in the board.
// Use to determine a draw condition if count is equal to CONFIG_BOARD_SPACES
uint16_t board_mark_count(void)
{
	return mark_count;
}
