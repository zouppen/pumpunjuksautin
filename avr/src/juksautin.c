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
#include <util/atomic.h>
#include "adc.h"
#include "pin.h"
#include "juksautin.h"
#include "hardware_config.h"

// With 1.1V AREF and 10-bit accuracy the conversion from samples to
// millivolts is 1100 / 1024 = 275 / 256.
#define MV_MULT 275
#define MV_DIV 256

// Store analog measurement sum and measurement count. Used for mean
// calculation.
typedef struct {
	uint32_t sum;
	uint16_t count;
} accu_t;

// Struct of accumulators
typedef struct {
	accu_t k9_raw;        // Real voltage in K9
	accu_t int_temp;      // AVR internal temperature
	accu_t outside_temp;  // Outside thermistor temp
	accu_t juksautin;     // Juksautin ratio
	int16_t err;          // Error led high value
} accus_t;

// Static prototypes
static accu_t take_accu(volatile accu_t *p);
static void handle_juksautus(uint16_t val);
static void handle_int_temp(uint16_t val);
static void handle_outside_temp(uint16_t val);
static void handle_err(uint16_t val);
static void store(volatile accu_t *a, uint16_t val);

// Static values
static volatile accus_t v_accu; // Holds all volatile measurement data
static volatile uint16_t target; // Target voltage for juksautus
static bool juksautus = false; // Is juksautus on at the moment?

void juksautin_init(void)
{
	adc_set_handler(0, handle_juksautus);
	adc_set_handler(8, handle_int_temp);
	adc_set_handler(3, handle_outside_temp);
	adc_set_handler(2, handle_err);
	juksautin_set_k9_target(1000);
}

void juksautin_set_k9_target(uint16_t const mv)
{
	uint16_t new_target = (uint32_t)mv * MV_DIV / MV_MULT;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		target = new_target;
	}
}

float juksautin_compute_k9_real_voltage(float mv, float ratio, float um, float rm)
{
	// See control.md
	return 1 / (((um / mv - 1) / rm) - (ratio / 200));
}

// Take (read and empty) analog accumulator.
static accu_t take_accu(volatile accu_t *p)
{
	accu_t a;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// Copy and empty it atomically first before heavy
		// floating point arithmetic.
		a = *p;
		a.sum = 0;
		a.count = 0;
	}
	return a;
}

// Defined in adc.h
uint8_t adc_channel_selection(void) {
	// Since this is called every ADC measurement, it's a good
	// place to record the ratio of pulldowns and total
	// measurements. That allows us to calculate the real
	// thermistor value while juksautus is happening. We calculate
	// juksautus count by counting the periods of time the current
	// is flowing.
	store(&v_accu.juksautin, juksautus);

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
	store(&v_accu.k9_raw, val);
}

static void handle_int_temp(uint16_t val)
{
	store(&v_accu.int_temp, val);
}

static void handle_outside_temp(uint16_t val)
{
	store(&v_accu.outside_temp, val);
}

static void handle_err(uint16_t val)
{
	if (v_accu.err >= val) return;
	v_accu.err = val;
}

// Update cumulative analog value for access outside the ISR. Do not let it overflow.
static void store(volatile accu_t *a, uint16_t val) {
	// Stop storing when full
	if (a->count == ~0) return;
	a->sum += val;
	a->count++;
}
