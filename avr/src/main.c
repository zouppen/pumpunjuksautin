// Pumpunjuksautin prototype for ATMega328p
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

// WIP WIP WIP WIP
#include "cmd.h"
#include "interface_ascii.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <util/atomic.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>

#include "pin.h"
#include "serial.h"
#include "clock.h"
#include "adc.h"
#include "hardware_config.h"

// Version definition is delivered by version.cmake
extern char const version[] PROGMEM;

// Store analog measurement sum and measurement count. Used for mean
// calculation.
typedef struct {
	uint32_t sum;
	uint16_t count;
} accu_t;

// Struct of accumulators.
typedef struct {
	accu_t k9_raw;    // Real voltage in K9
	accu_t int_temp;  // AVR internal temperature
	accu_t outside_temp;  // Outside thermistor temp
	accu_t juksautin; // Juksautin ratio
	accu_t err;       // Error led ratio
} accus_t;

// Prototypes
static void store(volatile accu_t *a, uint16_t val);
static void loop(void);
static void handle_juksautus(uint16_t val);
static void handle_int_temp(uint16_t val);
static void handle_outside_temp(uint16_t val);
static void handle_err(uint16_t val);
static float compute_real_temp(float mv, float ratio, float um, float rm);
static bool strip_line_ending(char *const buf, int const len);

#define VOLT (1.1f / 1024) // 1.1V AREF and 10-bit accuracy

volatile accus_t v_accu; // Holds all volatile measurement data
volatile uint16_t target; // Target voltage for juksautus
static bool juksautus = false; // Is juksautus on at the moment?

// Set juksautin target voltage.
void set_voltage(float u) {
	uint16_t new_target = u / 1.1 * 1024;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		target = new_target;
	}
}

// Initialization
int main() {
	// FB pin is toggled between HI-Z and LOW, prepare it. Start with HI-Z.
	LOW(PIN_FB);
	INPUT(PIN_FB);

	// Configure output pins
	OUTPUT(PIN_LED);

	// Initialize modules.
	serial_init();
	clock_init();
	adc_init();
	adc_set_handler(0, handle_juksautus);
	adc_set_handler(8, handle_int_temp);
	adc_set_handler(3, handle_outside_temp);
	adc_set_handler(2, handle_err);

	set_voltage(1);

	// Set ADSC in ADCSRA (0x7A) to start the ADC conversion
	adc_start_sourcing(8);

	// Enable global interrupts.
	sei();

	while (true) {
		loop();
		sleep_mode();
	}
}

// Take analog accumulator mean value
float accu_mean(accu_t *a) {
	return (float)a->sum / a->count;
}

// Processor loop
void loop(void) {
	// Continue only if transmitter is idle and we have a new
	// frame to parse.
	if (serial_is_transmitting()) return;

	char *rx_buf;
	buflen_t len = serial_get_message(&rx_buf);

	// Continue only if we have got a message.
	if (rx_buf == NULL) return;

	// Incoming message counter
	static uint16_t i = ~0;
	i++;

	// Marker if any serial is written.
	*serial_tx = '\0';

	// Process the message
	bool ok = interface_ascii(rx_buf, len);
	serial_free_message();

	// Send the answer if we have one
	if (*serial_tx != '\0') {
	    serial_tx_line();
	}
}

static float compute_real_temp(float mv, float ratio, float um, float rm)
{
	// See control.md
	return 1 / (((um / mv - 1) / rm) - (ratio / 200));
}

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
	store(&v_accu.err, val);
}

uint8_t adc_channel_selection(void) {
	// Since this is called every ADC measurement, it's a good
	// place to record the ratio of pulldowns and total
	// measurements. That allows us to calculate the real
	// thermistor value while juksautus is happening. We calculate
	// juksautus count by counting the periods of time the current
	// is flowing.
	store(&v_accu.juksautin, juksautus);

	// Now the actual selection. We use cycle length of 16
	uint8_t cycle = v_accu.juksautin.count & 0b1111;
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

// Update cumulative analog value for access outside the ISR. Do not let it overflow.
static void store(volatile accu_t *a, uint16_t val) {
	// Stop storing when full
	if (a->count == ~0) return;
	a->sum += val;
	a->count++;
}
