#include <util/atomic.h>
#include "clock.h"

/*
   Use dividers and counters to get 1 second from the clock
   frequency. You need to get F_CPU = prescaler * TOP_A * TOP_B.

   In our case prescaler is 256 and TOP_A is 250, we generate
   Counter2 compare match A interrupt every 250x256 cycles which 4 ms
   when F_CPU is 16MHz. It is then counted to full seconds by an
   additional 250 counter TOP_B.

   Counter2 compare match B interrupt is available for accuracy
   timing, having granularity of prescaler / F_CPU = 16µs
*/

#define TOP_A 250 // Counter TOP value in OCR2A
#define TOP_B 250 // Software divider

typedef struct {
	callback_t callback;
	uint8_t timeout;
} oneshot_t;

static volatile uint8_t counter_b = TOP_B;
static oneshot_t oneshot[ONESHOT_COUNT];
static bool is_set = false;

void clock_init(void)
{
	// CTC with OCRA as TOP.
	TCCR2A |= _BV(WGM21);
	// We get an interrupt every prescaler * TOP_A cycles.
	OCR2A = TOP_A-1;
	// Enable Timer Compare match A interrupt.
	TIMSK2 |= _BV(OCIE2A);
	// Prescaler F_CPU / 256.
	TCCR2B |= _BV(CS22) | _BV(CS21);
}

void clock_set(time_t now, int32_t zone)
{
	now -= UNIX_OFFSET;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// Reset counters
		TCNT2 = 0;
		counter_b = 0;
		set_system_time(now);
		set_zone(zone);
		is_set = true;
	}
}

bool clock_is_set(void)
{
	return is_set;
}

void clock_setup_timer(int slot, callback_t callback)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		oneshot[slot].callback = callback;
		oneshot[slot].timeout = 0;
	}
}

void clock_arm_timer(int slot, uint16_t timeout_ms)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// Timer decrements every 4 milliseconds
		oneshot[slot].timeout = (timeout_ms >> 2) + 1;
	}
}	

// Timer interrupt increments counter_b until full second is elapsed.
ISR(TIMER2_COMPA_vect)
{
	// System time
	counter_b--;
	if (counter_b == 0) {
		system_tick();
		counter_b = TOP_B;
	}

	// Timers
	for (int i=0; i<(sizeof(oneshot)/sizeof(oneshot_t)); i++) {
		oneshot_t *a = &oneshot[i];
		if (a->timeout == 0) {
			continue;
		}
		a->timeout--;
		if (a->timeout == 0) {
			a->callback();
		}
	}
}
