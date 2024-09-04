#ifndef GAMECONTROL_H_
#define GAMECONTROL_H_

// Initialize the game control logic.
// This function initializes all missiles, planes, stats, etc.
void gameControl_init(void);

// Update the game control logic.
// This function calls the missile & plane tick functions, reinitializes
// idle enemy missiles, handles button presses, fires player missiles,
// detects collisions, and updates statistics.
void gameControl_tick(void);

#endif // GAMECONTROL_H_
