// Pumpunjuksautin main object.
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

#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <util/atomic.h>
#include "adc.h"
#include "serial.h"
#include "clock.h"
#include "juksautin.h"
#include "pin.h"
#include "hardware_config.h"
#include "interface/ascii.h"
#include "interface/modbus.h"

// Prototypes
static void loop(void);

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
	juksautin_init();
	adc_init();

	// Start ADC loop by reading any channel
	adc_start_sourcing(8);

	// Enable global interrupts.
	sei();

	while (true) {
		loop();

		// CPU sleeps until interrupts occur.
		sleep_mode();
	}
}

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

	const bool overflow = len > SERIAL_RX_LEN;
	// Modbus function code is never an ASCII letter so it can be
	// used to check if the message is textual.
	const bool is_ascii = len >= 2 && isalpha(rx_buf[1]);
	const bool ascii_allowed = WITH_ASCII && (!WITH_MODBUS || is_ascii);
	
	if (overflow) {
		serial_free_message();
		// Be silent about the error if there's a risk of
		// shouting over another client.
		if (ascii_allowed) {
			snprintf_P(serial_tx, SERIAL_TX_LEN, PSTR("Incoming message longer than %d bytes"), SERIAL_RX_LEN);
			serial_tx_line();
		}
	} else if (ascii_allowed) {
		// Process ASCII message
		ascii_interface(rx_buf, len);
		serial_free_message();
		serial_tx_line();
	} else if (WITH_MODBUS) {
		// Process Modbus message
		buflen_t tx_len = modbus_interface(rx_buf, len);
		serial_free_message();
		if (tx_len != 0) {
			serial_tx_bin(tx_len);
		}
	} else {
		serial_free_message();
	}
}
