#include <time.h>
#include <stdbool.h>

#define ONESHOT_COUNT 1

typedef void (*callback_t)(void);

// Initialize clock using TIMER2
void clock_init(void);

// Set clock to given time. NB! Resets also internal tick counters.
void clock_set(time_t now, int32_t zone);

// Test if clock is already set or is it running fake time
bool clock_is_set(void);

// Setup callback for given timer slot.
void clock_setup_timer(int slot, callback_t callback);

// Arm timer. Timeout given in milliseconds but they have accuracy of
// prescaer * TOP_A / F_CPU = 4 ms.
void clock_arm_timer(int slot, uint16_t timeout_ms);
