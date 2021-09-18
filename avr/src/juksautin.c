// Pumpunjuksautin core functions. The business logic.
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

#include <stdbool.h>
#include <stdint.h>
#include <avr/eeprom.h>
#include <util/atomic.h>
#include "adc.h"
#include "pin.h"
#include "juksautin.h"
#include "hardware_config.h"

// With 1.1V AREF and 10-bit accuracy the conversion from samples to
// millivolts is 1100 / 1024 = 275 / 256.
#define MV_MULT 275
#define MV_DIV 256

// To ensure we have enough headroom in the 32 bit voltage sum field
// we need to keep the value below this. Value of 1024 is substracted
// to make space for another 10-bit sample = 2^10 in case the counter
// is odd.
static uint32_t const accu_mv_sum_max = ~(uint32_t)0 / MV_MULT - 1024;

// The same as in above but with scaling to 2^16 = 65536. And space for
// another "1-bit sample" = 2^1 = 2.
static uint32_t const accu_bool_sum_max = 65534;

// Store analog measurement sum and measurement count. Used for mean
// calculation.
typedef struct {
	uint32_t sum;
	uint16_t count;
} accu_t;

// Struct of accumulators
typedef struct {
	accu_t k5_raw;           // Real voltage in K5
	accu_t int_temp;         // AVR internal temperature
	accu_t outside_temp;     // Outside thermistor temp
	accu_t accumulator_temp; // Accumulator tank thermistor temp
	accu_t juksautin;        // Juksautin ratio
	int16_t err;             // Error led high value
} accus_t;

// Static prototypes
static uint16_t to_millivolts(accu_t a);
static uint16_t to_ratio16(accu_t a);
static accu_t take_accu(volatile accu_t *p);
static void handle_juksautus(uint16_t val);
static void handle_int_temp(uint16_t val);
static void handle_outside_temp(uint16_t val);
static void handle_accumulator_temp(uint16_t val);
static void handle_err(uint16_t val);
static void store(volatile accu_t *a, uint16_t const val, uint32_t const max);

// Static values
static volatile accus_t v_accu; // Holds all volatile measurement data
static volatile uint16_t target; // Target voltage for juksautus
static uint16_t ee_target EEMEM = 1000l * MV_DIV / MV_MULT; // EEPROM initial value is 1 V

// Not volatile because used only inside ISRs
static bool juksautus = false; // Is juksautus on at the moment?

void juksautin_init(void)
{
	adc_set_handler(0, handle_juksautus);
	adc_set_handler(8, handle_int_temp);
	adc_set_handler(3, handle_outside_temp);
	adc_set_handler(4, handle_accumulator_temp);
	adc_set_handler(2, handle_err);

	// Retrieve target from EEPROM
	target = eeprom_read_word(&ee_target);
}

modbus_status_t juksautin_set_target(uint16_t const mv)
{
	uint16_t new_target = ((uint32_t)mv * MV_DIV + MV_DIV / 2) / MV_MULT;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		target = new_target;
	}
	eeprom_update_word(&ee_target, new_target);
	return MODBUS_OK;
}

uint16_t juksautin_get_target(void)
{
	uint32_t raw;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		raw = target;
	}
	return (raw * MV_MULT + MV_MULT / 2) / MV_DIV;
}

float juksautin_compute_k5_real_voltage(float mv, float ratio, float um, float rm)
{
	// See control.md
	return 1 / (((um / mv - 1) / rm) - (ratio / 200));
}

uint16_t juksautin_take_k5_raw_mv(void)
{
	return to_millivolts(take_accu(&v_accu.k5_raw));
}

uint16_t juksautin_take_ratio(void)
{
	return to_ratio16(take_accu(&v_accu.juksautin));
}

uint16_t juksautin_take_error(void)
{
	uint16_t error;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// Copy and empty it atomically.
		error = v_accu.err;
		v_accu.err = 0;
	}
	return error;
}

uint16_t juksautin_take_outside_temp(void)
{
	return to_millivolts(take_accu(&v_accu.outside_temp));
}

uint16_t juksautin_take_accumulator_temp(void)
{
	return to_millivolts(take_accu(&v_accu.accumulator_temp));
}

// Take (read and empty) analog accumulator.
static accu_t take_accu(volatile accu_t *p)
{
	accu_t a;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// Copy and empty it atomically.
		a = *p;
		p->sum = 0;
		p->count = 0;
	}
	return a;
}

// Convert accumulator value to arithmetic mean millivolts.
static uint16_t to_millivolts(accu_t a)
{
	return a.sum * MV_MULT / a.count / MV_DIV;
}

// Convert accumulator value to ratio scaled up to full scale of 16
// bit unsigned integer.
static uint16_t to_ratio16(accu_t a)
{
	return (a.sum << 16) / a.count;
}

// Defined in adc.h
uint8_t adc_channel_selection(void)
{
	// Since this is called every ADC measurement, it's a good
	// place to record the ratio of pulldowns and total
	// measurements. That allows us to calculate the real
	// thermistor value while juksautus is happening. We calculate
	// juksautus count by counting the periods of time the current
	// is flowing.
	store(&v_accu.juksautin, juksautus, accu_bool_sum_max);

	// Now the actual selection. We use cycle length of 16
	static uint8_t cycle = 0;
	cycle = (cycle+1) & 0b1111;
	switch (cycle) {
	case 0:
		// Internal temperature measurement
		return 8;
	case 4:
		// Error LED sampling
		return 2;
	case 8:
		// Outside temperature measurement
		return 3;
	case 12:
		// Tank temperature measurement
		return 4;
	default:
		// NTC thermistor measurement
		return 0;
	}
}

// Now follows ADC measurement handlers. Thoese functions are called
// from ADC interrupt handler.

static void handle_juksautus(uint16_t val)
{
	// Pump logic. Pull capacitor down to target voltage.
	juksautus = val > target;
	if (juksautus) {
		OUTPUT(PIN_FB);
	} else {
		INPUT(PIN_FB);
	}

	// Store measurement
	store(&v_accu.k5_raw, val, accu_mv_sum_max);
}

static void handle_int_temp(uint16_t val)
{
	store(&v_accu.int_temp, val, accu_mv_sum_max);
}

static void handle_outside_temp(uint16_t val)
{
	store(&v_accu.outside_temp, val, accu_mv_sum_max);
}

static void handle_accumulator_temp(uint16_t val)
{
	store(&v_accu.accumulator_temp, val, accu_mv_sum_max);
}

static void handle_err(uint16_t val)
{
	if (v_accu.err >= val) return;
	v_accu.err = val;
}

// Update cumulative analog value for access outside the ISR.
static void store(volatile accu_t *a, uint16_t const val, uint32_t const max)
{
	a->sum += val;
	a->count++;
	// Handle overflows in both divident and divisor
	if (a->count == 0) {
		// When we run out of headroom in counter, throw the
		// least significant bits away.
		a->sum >>= 1;
		a->count = 1 << 15;
	} else if ((a->count & 1) == 0 && a->sum > max) {
		// When the sum has higher value than the millivolt
		// conversion can handle AND the count is even, let's
		// throw throw least significant bits away to get some
		// more headroom. This happens about once per minute
		// if there are no calls to take_accu().
		a->sum >>= 1;
		a->count >>= 1;
	}
}
