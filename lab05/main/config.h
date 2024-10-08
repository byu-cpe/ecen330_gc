#ifndef CONFIG_H_
#define CONFIG_H_

#define CONFIG_GAME_TIMER_PERIOD 40.0E-3f

// Board
#define CONFIG_BOARD_R 3 // Rows
#define CONFIG_BOARD_C 3 // Columns
#define CONFIG_BOARD_N 3 // Number of contiguous marks
// #define CONFIG_BOARD_R 5 // Rows
// #define CONFIG_BOARD_C 7 // Columns
// #define CONFIG_BOARD_N 4 // Number of contiguous marks

#define CONFIG_BOARD_SPACES (CONFIG_BOARD_R*CONFIG_BOARD_C)

// Colors
#define CONFIG_BACK_CLR rgb565(0, 16, 42)
#define CONFIG_GRID_CLR WHITE
#define CONFIG_MARK_CLR YELLOW
#define CONFIG_HIGH_CLR GREEN
#define CONFIG_MESS_CLR CYAN

#endif // CONFIG_H_
