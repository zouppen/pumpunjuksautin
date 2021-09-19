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
#include <avr/eeprom.h>
#include "clock.h"

// avr_libc internal variable
extern const long __utc_offset;

static int unixy_dst(const time_t *time, int32_t *z);
static void update_tzdata_from_eeprom(void);

static volatile uint8_t counter_b = CLOCK_B;
static bool is_set = false;

// DST changes data (avr = AVR epoch, not UNIX)
static time_t clock_turn_avr;
static int32_t clock_turn_offset;

// These variables are stored in EEPROM. Zone variables are signed
// despite of the data type. Parameters zone_now and zone_turn are in
// "gmtoff" format, i.e. seconds east to UTC, e.g. 3600 for
// UTC+1. Parameter zone_now is the currently observed gmtoffime
// zone. Parameter ts_turn defines when the clocks turn the next
// time. Parameter zone_turn is the gmtoff observed after that
// moment. In case of no known future DST changes, ts_turn must be 0.
// These are uints for compatibility. In practice zone variables are
// signed, but that's taken care in update_tzdata_from_eeprom().
static uint32_t ee_zone_now EEMEM = 0;
static uint32_t ee_ts_turn EEMEM = 0;
static uint32_t ee_zone_turn EEMEM = 0;

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

	// Read current timezone data from EEPROM
	update_tzdata_from_eeprom();

	// Use our own function for summer time math
	set_dst(unixy_dst);
}

modbus_status_t clock_set_time_unix(time_t const ts_now)
{
	// AVR uses Zigbee epoch. Converting from UNIX epoch
	time_t const avr_now = ts_now - UNIX_OFFSET;

	// Make sure we do the clock update stuff atomically
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// Reset counters. Not resetting TIMER2 because it
		// would interfers with UART delay timers.
		counter_b = 0;
		set_system_time(avr_now);
		is_set = true;
	}
	return MODBUS_OK;
}

bool clock_is_set(void)
{
	return is_set;
}

// Reads time zone data from EEPROM and apply it to the system. See
// ee_* definitions above for how to set the data.
static void update_tzdata_from_eeprom(void)
{
	// Collect data from persistent memory
	int32_t const zone_now = eeprom_read_dword(&ee_zone_now);
	time_t const ts_turn = eeprom_read_dword(&ee_ts_turn);
	int32_t const zone_turn = eeprom_read_dword(&ee_zone_turn);

	// DST structure in UNIX and AVR-libc are different. Unix uses
	// time zone offset directly but avr-libc thinks traditionally
	// by using "base zone" and supports positive DST offsets
	// only. Therefore we need to mangle the data a bit.

	// Current zone is smaller of the two, because DST offset must
	// be positive. When no DST, use zone_now.
	int32_t const pseudo_zone = ts_turn == 0 || zone_now < zone_turn ? zone_now : zone_turn;

	// Calculate difference of current and future zones. In case
	// of no DST (ts_turn is 0) make sure the offset is zero, too.
	int32_t const new_turn_offset = ts_turn == 0 ? 0 : zone_turn - zone_now;

	// AVR uses Zigbee epoch. Converting from UNIX epoch
	time_t const new_turn_avr = ts_turn - UNIX_OFFSET;

	// Make sure we do the clock update stuff atomically
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		set_zone(pseudo_zone);
		clock_turn_avr = new_turn_avr;
		clock_turn_offset = new_turn_offset;
	}
}

modbus_status_t clock_set_gmtoff(int32_t gmtoff)
{
	eeprom_update_dword(&ee_zone_now, gmtoff);
	update_tzdata_from_eeprom();
	return MODBUS_OK;
}

modbus_status_t clock_set_next_turn(uint32_t ts)
{
	eeprom_update_dword(&ee_ts_turn, ts);
	update_tzdata_from_eeprom();
	return MODBUS_OK;
}

modbus_status_t clock_set_gmtoff_turn(int32_t gmtoff)
{
	eeprom_update_dword(&ee_zone_turn, gmtoff);
	update_tzdata_from_eeprom();
	return MODBUS_OK;
}

time_t clock_get_time_unix()
{
	return time(NULL) + UNIX_OFFSET;
}

int32_t clock_get_gmtoff()
{
	time_t now;
	time(&now);
	return __utc_offset + unixy_dst(&now, NULL);
}

uint32_t clock_get_next_turn()
{
	return eeprom_read_dword(&ee_ts_turn);
}

int32_t clock_get_gmtoff_turn()
{
	return eeprom_read_dword(&ee_zone_turn);
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

// This DST handler can only handle present and future DST changes
// over one DST change. Meaning it doesn't work as expected for past
// dates and if the time is more than couple of months in
// advance. With usual control tasks it's not a problem.
static int unixy_dst(const time_t *time, int32_t *z)
{
	bool const before_turn = *time < clock_turn_avr;
	bool const towards_summer = clock_turn_offset > 0;
	bool const is_now_dst = before_turn ^ towards_summer;

	return is_now_dst ? labs(clock_turn_offset) : 0;
}
