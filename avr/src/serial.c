// Pumpunjuksautin RS-485 interface
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


// To understand what happens here, please read ATmega328p data sheet
// chapter 19 (USART0) and ESPECIALLY subchapter 19.6.3 (Transmitter
// Flags and Interrupts).

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <util/atomic.h>
#include <util/setbaud.h>

#include "serial.h"
#include "clock.h"
#include "pin.h"
#include "hardware_config.h"

typedef enum {
	rx_active, // Receiving data
	rx_end, // Between rx packets
	rx_tx_ready // We could even tx
} rx_state_t;

char serial_tx[SERIAL_TX_LEN]; // Outgoing serial data

// Double buffering for rx
static uint8_t serial_rx_a[SERIAL_RX_LEN]; // Receive buffer a
static uint8_t serial_rx_b[SERIAL_RX_LEN]; // Receive buffer b
static uint8_t *serial_rx_back = serial_rx_a; // Back buffer (for populating data)
static uint8_t *serial_rx_front = NULL; // Contains front buffer if it's not yet freed
static uint8_t serial_rx_front_len; // Contains front buffer data length
static uint8_t serial_tx_len; // Total number of bytes to send

static uint8_t serial_rx_i = 0; // Receive buffer position.
				// In case of an overflow it will be ~0.
static uint8_t serial_tx_i = 0; // Send buffer position
static volatile rx_state_t rx_state = rx_tx_ready;
static volatile bool tx_state = false; // Is tx start requested

// (Error) counters
static volatile serial_counter_t counts = {0, 0, 0};

// Static prototypes
static void transmit_now(void);
static void end_of_frame(void);

void serial_init(void)
{
	// Initialize UART. Baud rate is defined by CMake variable
	// "BAUD". See more information:
	// https://www.nongnu.org/avr-libc/user-manual/group__util__setbaud.html
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
#if USE_2X
	UCSR0A |= _BV(U2X0);
#else
	UCSR0A &= ~_BV(U2X0);
#endif

	// Prepare for RS-485 half-duplex. Start in RX mode (LOW).
	OUTPUT(PIN_TX_EN);

	// Turn on all serial interrupts except UDRIE which is turned
	// when we want to transmit.
	UCSR0B = _BV(RXEN0) | // Receive enable
		_BV(RXCIE0) | // Enable USART_RX_vect
		_BV(TXEN0)  | // Transmitter enable
		_BV(TXCIE0);  // Enable USART_TX_vect
}

bool serial_is_transmitting(void) {
	return tx_state;
}

void serial_tx_line(void) {
	// Search for terminating NUL.
	uint8_t len = strnlen(serial_tx, SERIAL_TX_LEN);

	// Not going to send any data which is not NULL terminated.
	if (len == SERIAL_TX_LEN) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			counts.too_long_tx++;
		}
		return;
	}

	// The message might have been cut. Indicate it with '>' sign.
	if (len+1 >= SERIAL_TX_LEN) {
		serial_tx[SERIAL_TX_LEN-2] = '>';
	}
	
	// Place newline at the end (overriding NUL) and send it.
	serial_tx[len] = '\n';
	serial_tx_bin(len+1);
}

void serial_tx_bin(uint8_t const len) {
	// The buffer is allowed to be completely full, that's why
	// comparing greater than instead of greater-than-equals.
	if (len > SERIAL_TX_LEN) {
		// Ignore the whole frame if it's too long.
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			counts.too_long_tx++;
		}
		return;
	}

	if (len == 0) {
		// Must not go to transmit phase at all.
		return;
	}

	// Use supplied length
	serial_tx_len = len;

	// RS-485 is half duplex. We need to wait until rx line
	// becomes idle.  We start by going to ready-to-send condition
	// and then check send if the receiver is idle. If it's not,
	// interrupt TIMER2_COMPB_vect will call transmit_now() later.
	tx_state = true;
	if (rx_state == rx_tx_ready) transmit_now();
}

// Callback when clock_arm_timer triggered
ISR(TIMER2_COMPB_vect) {
	// Timer is oneshot, cancel timer
	TIMSK2 &= ~_BV(OCIE2B);

	if (rx_state == rx_active) {
		// Phase 1: End of frame. Restart timer for phase 2.
		rx_state = rx_end;
		clock_arm_timer(MODBUS_SILENCE);
		end_of_frame();
	} else {
		// Phase 2: Ready to transmit
		rx_state = rx_tx_ready;
		if (tx_state) transmit_now();
	}
}

// Called from timer interrupt handler when we're between frames.
static void end_of_frame(void)
{
	bool const overflow = serial_rx_i > SERIAL_RX_LEN;
	bool locked = serial_rx_front != NULL;
	if (overflow) {
		// Too long frame is completely
		// ignored. Rollback the buffer and prepare
		// for a new frame.
		counts.too_long_rx++;
	} else if (locked) {
		// We need to throw a frame overboard
		// because main loop didn't process
		// front buffer in time.
		counts.flip_timeout++;
	} else {
		// Looks good, at least before CRC checks and so.
		counts.good++;

		// Collect length for later use
		serial_rx_front_len = serial_rx_i;

		// Flip buffers!
		serial_rx_front = serial_rx_back;
		serial_rx_back =
			(serial_rx_a == serial_rx_back)
			? serial_rx_b
			: serial_rx_a;
	}
	// Latest but not least: Rewind receive buffer
	serial_rx_i = 0;
}

static void transmit_now()
{
	// Indicator only.
	TOGGLE(PIN_LED);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// RS-485 direction change.
		HIGH(PIN_TX_EN);
		
		// Let the TX interrupt run.
		UCSR0B |= _BV(UDRIE0); // Enable USART_UDRE_vect
	}
}

uint8_t serial_get_message(uint8_t **const buf)
{
	int len;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (serial_rx_front == NULL) {
			*buf = NULL;
			return 0;
		}

		serial_rx_front[serial_rx_front_len] = '\0';

		*buf = serial_rx_front;
		len = serial_rx_front_len;
	}
	return len;
}

void serial_free_message(void) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		serial_rx_front = NULL;
	}
}

serial_counter_t pull_serial_counters(void) {
	serial_counter_t ret;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		ret = counts;
		counts.good = 0;
		counts.flip_timeout = 0;
		counts.too_long_rx = 0;
		counts.too_long_tx = 0;
	}
	return ret;
}

// Transmit finished. This interrupt is called after all data has been
// sent (UDRE_vect no longer feeds data). In this interrupt the RS-485
// is switched back to receive mode.
ISR(USART_TX_vect)
{
	// Indicator only.
	TOGGLE(PIN_LED);

	// Rollback transmit buffer.
	serial_tx_i = 0;

	// RS-485 direction change.
	LOW(PIN_TX_EN);

	// Ready to transmit again.
	tx_state = false;
}

// Called when there is opportunity to fill TX FIFO.
ISR(USART_UDRE_vect)
{
	uint8_t const out = serial_tx[serial_tx_i];

	if (serial_tx_len == serial_tx_i + 1) {
		// We are going to transmit the last
		// character. Disable this interrupt.
		UCSR0B &= ~_BV(UDRIE0);
	}

	// Transmit
	UDR0 = out;
	serial_tx_i++;
}

// Called when data available from serial.
ISR(USART_RX_vect)
{
	// Arm the timer when we receive a character. When
	// MODBUS_SILENCE amount of ticks is passed, consider a
	// complete frame.
	clock_arm_timer(MODBUS_SILENCE);
	rx_state = rx_active;

	uint8_t in = UDR0;

	// Using >= in comparison instead of > because we got a
	// character after the buffer is already full (no place to put
	// that character any more).
	bool const overflow = serial_rx_i >= SERIAL_RX_LEN;
	if (overflow) {
		// We got a character after the buffer is
		// full. Marking the index as invalid.
		serial_rx_i = ~0;
		return;
	}

	serial_rx_back[serial_rx_i] = in;
	serial_rx_i++;
}
