#include <time.h>
#include <stdbool.h>

// Initialize clock using TIMER2
void clock_init(void);

// Set clock to given time. NB! Resets also internal tick counters.
void clock_set(time_t now, int32_t zone);

// Test if clock is already set or is it running fake time
bool clock_is_set(void);
