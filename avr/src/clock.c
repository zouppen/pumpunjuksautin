// Pumpunjuksautin clock handling for ATMega328p
// SPDX-License-Identifier:   GPL-3.0-or-later
// Copyright (C) 2021 Joel Lehtonen
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <util/atomic.h>
#include "clock.h"

static volatile uint8_t counter_b = CLOCK_B;
static bool is_set = false;

void clock_init(void)
{
	// CTC with OCRA as TOP.
	TCCR2A |= _BV(WGM21);
	// We get an interrupt every prescaler * CLOCK_A cycles.
	OCR2A = CLOCK_A-1;
	// Enable Timer Compare match A interrupt.
	TIMSK2 |= _BV(OCIE2A);

	// Setting prescaler. For readability we are not using macros
	// but costant values. To understand this, see ATmega328p data
	// sheet table 17-9. Clock Select Bit Description.
#if CLOCK_PRESCALER == 1
	TCCR2B |= 1;
#elif CLOCK_PRESCALER == 8
	TCCR2B |= 2;
#elif CLOCK_PRESCALER == 32
	TCCR2B |= 3;
#elif CLOCK_PRESCALER == 64
	TCCR2B |= 4;
#elif CLOCK_PRESCALER == 128
	TCCR2B |= 5;
#elif CLOCK_PRESCALER == 256
	TCCR2B |= 6;
#elif CLOCK_PRESCALER == 1024
	TCCR2B |= 7;
#else
#error Prescaler must be one of: 1, 8, 32, 64, 128, 256, 1024.
#endif
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

void clock_arm_timer(uint8_t delay)
{
	// To avoid difficult to handle integer overflows, we are
	// using 16 bit integer for the math and then do integer
	// modulo operation.
	uint16_t target = (uint16_t)TCNT2 + delay;
	if (target >= CLOCK_A) target -= CLOCK_A;

	// Set timer B match to given target.
	OCR2B = target;

	// Clear interrupt flag
	TIFR2 |= _BV(OCF2B);
	
	// Enable Timer Compare match B interrupt.
	TIMSK2 |= _BV(OCIE2B);
}

// Timer interrupt increments counter_b until full second is elapsed.
ISR(TIMER2_COMPA_vect)
{
	counter_b--;
	if (counter_b == 0) {
		system_tick();
		counter_b = CLOCK_B;
	}
}
