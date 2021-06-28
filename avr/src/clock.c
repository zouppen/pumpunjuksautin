#include <util/atomic.h>

#include "clock.h"

static volatile uint8_t clock_4ms = 0;
static bool is_set = false;

void clock_init(void) {
	// CTC with OCRA as TOP
	TCCR2A |= _BV(WGM21);
	// Generate interrupt every 250x256 cycles, which gives clock
	// granularity of 4ms @ 16MHz
	OCR2A = 250-1;
	// Enable Timer Compare match A interrupt
	TIMSK2 |= _BV(OCIE2A);
	// Prescaler clk_io / 256
	TCCR2B |= _BV(CS22) | _BV(CS21);
}

void clock_set(time_t now, int32_t zone) {
	now -= UNIX_OFFSET;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// Reset counters
		TCNT2 = 0;
		clock_4ms = 0;
		set_system_time(now);
		set_zone(zone);
		is_set = true;
	}
}

bool clock_is_set(void) {
	return is_set;
}

/**
 * Timer interrupt increments RTC
 */
ISR(TIMER2_COMPA_vect)
{
	if (clock_4ms == 250-1) {
		system_tick();
		clock_4ms = 0;
	} else {
		clock_4ms++;
	}
}
