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

// Note, many macro values are defined in <avr/io.h> and
// <avr/interrupts.h>, which are included automatically by
// the Arduino interface

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
	static uint16_t i = ~0;
	time_t now;
	int zone_h, zone_m;
	uint16_t target_millivoltage;

	// Continue only if transmitter is idle and we have a new
	// frame to parse.
	if (serial_is_transmitting()) return;

	// Using char* because of strcmp & friends. serial_get_message()
	// is giving uint8_t* but it's less error prone to suppress
	// warnings than to do an unconditional typecast.
	char *rx_buf;
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
	int len = serial_get_message(&rx_buf);
#pragma GCC diagnostic pop
	if (rx_buf == NULL) return;

	i++;

	// Process only messages with a newline
	if (rx_buf[len-1] != '\n') {
		serial_free_message();
		strcpy_P(serial_tx, PSTR("Message not terminated by newline"));
		serial_tx_start();
		return;
	}

	// Clean trailing CR and LF
	rx_buf[len-1] = '\0';
	if (len >= 2 && rx_buf[len-2] == '\r') {
		rx_buf[len-2] = '\0';
	}

	// Parse command. NB! Call serial_free_message() ASAP after
	// rx_buf is no longer peeked to avoid timeouts.
	if (strcmp_P(rx_buf, PSTR("PING")) == 0) {
		serial_free_message();
		// Prepare ping answer
		strcpy_P(serial_tx, PSTR("PONG"));
		serial_tx_start();
	} else if (strcmp_P(rx_buf, PSTR("LED")) == 0) {
		serial_free_message();
		// Useful for testing if rx works because we see
		// visual indication even if tx is bad.
		TOGGLE(PIN_LED);
	} else if (strcmp_P(rx_buf, PSTR("READ")) == 0) {
		serial_free_message();
		// Duplicate the data
		accus_t accu;
		ATOMIC_BLOCK(ATOMIC_FORCEON) {
			// The buffer is so small it makes more sense to use copying than
			// front/back buffering at the moment.
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
			// We are safely fiddling with volatile, so we just
			// turn off warnings at this stage.
			memcpy(&accu, &v_accu, sizeof(accu));
			memset(&v_accu, 0, sizeof(v_accu));
#pragma GCC diagnostic pop
		}

		float ratio = accu_mean(&accu.juksautin);
		float int_temp = (accu_mean(&accu.int_temp)-324.31)/1.22;
		float outside_temp = (accu_mean(&accu.outside_temp)) * VOLT;
		float k9_raw = accu_mean(&accu.k9_raw) * VOLT;
		float err_ratio = accu_mean(&accu.err);

		int wrote = snprintf_P(serial_tx,
				     SERIAL_TX_LEN, PSTR("%" PRIu16 ": internal: %d°C\noutside: %dmV %dohm\ntank: %dmv %dohm %d%% %" PRIu16 "\nError: %f"),
				     i, 
				     (int)int_temp, 
				     (int)(outside_temp * 1000), 
				     (int)compute_real_temp(outside_temp, 0, 0.822, 4434),
				     (int)(k9_raw*1000), 
				     (int)compute_real_temp(k9_raw, ratio, 0.944, 1516),
				     (int)(ratio*100), 
				     accu.juksautin.count,
				     err_ratio);
	
		if (wrote >= SERIAL_TX_LEN) {
			// Ensuring endline in the end
			serial_tx[SERIAL_TX_LEN-1] = '\0';
		}
		serial_tx_start();
	} else if (strcmp_P(rx_buf, PSTR("TIME")) == 0) {
		serial_free_message();
		// Get time
		time(&now);
		strftime(serial_tx, SERIAL_TX_LEN, "%F %T%z", localtime(&now));
		serial_tx[SERIAL_TX_LEN-1] = '\0'; // Ensure null termination
		serial_tx_start();
	} else if (sscanf_P(rx_buf, PSTR("VOLTAGE %" SCNu16), &target_millivoltage) == 1) {
		serial_free_message();
		float target_voltage = (float)target_millivoltage / 1000;
		set_voltage(target_voltage);
		snprintf_P(serial_tx, SERIAL_TX_LEN, PSTR("Set voltage to %f V"), target_voltage);
		serial_tx[SERIAL_TX_LEN-1] = '\0'; // Ensure null termination
		serial_tx_start();
	} else if (sscanf_P(rx_buf, PSTR("TIME %lu%3d%d"), &now, &zone_h, &zone_m) == 3) {
		serial_free_message();
		// Set time
		clock_set(now, ((int32_t)zone_h*60+zone_m)*60);
	} else if (strcmp_P(rx_buf, PSTR("VERSION")) == 0) {
		serial_free_message();
		strcpy_P(serial_tx, version);
		serial_tx_start();
	} else {
		// Do not answer to unrelated messages. This is
		// important to handle point-to-multipoint protocol:
		// We are not answering packets not related to us.

		// Dump everything. This is temporary to ease debugging.
		char *out = serial_tx;
		strcpy_P(serial_tx, PSTR("Invalid data: "));
		out += 14;

		for (int i=0; rx_buf[i] != '\0'; i++) {
			out += snprintf(out, serial_tx + SERIAL_TX_LEN - out, "%02hhx ", rx_buf[i]);

			if (serial_tx + SERIAL_TX_LEN - out <= 0) {
				// Ensure it's null terminated and stop!
				serial_tx[SERIAL_TX_LEN-3] = '.';
				serial_tx[SERIAL_TX_LEN-2] = '.';
				serial_tx[SERIAL_TX_LEN-1] = '\0';
				break;
			}
		}

		serial_free_message();
		serial_tx_start();
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
