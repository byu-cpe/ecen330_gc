#ifndef WATCH_H_
#define WATCH_H_

#include <stdint.h>

// Initialize the watch face.
void watch_init(void);

// Update the watch digits based on timer_ticks (1/100th of a second).
void watch_update(uint32_t timer_ticks);

#endif // WATCH_H_
