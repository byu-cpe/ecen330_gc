#ifndef GAME_H_
#define GAME_H_

// Initialize the game control logic.
// This function initializes all missiles, planes, stats, etc.
void game_init(void);

// Update the game control logic.
// This function calls the missile & plane tick functions, reinitializes
// idle enemy missiles, handles button presses, fires player missiles,
// detects collisions, and updates statistics.
void game_tick(void);

#endif // GAME_H_
