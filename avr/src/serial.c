// RS-485 interface
//
// To understand what happens here, please read ATmega328p data sheet
// chapter 19 (USART0) and ESPECIALLY subchapter 19.6.3 (Transmitter
// Flags and Interrupts).

#include <stdlib.h>
#include <stdbool.h>
#include <util/atomic.h>
#include <util/setbaud.h>

#include "serial.h"
#include "clock.h"
#include "pin.h"
#include "hardware_config.h"

#define SERIAL_RX_LEN 30

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

// RS-485 is half duplex. We need to wait until the line is tx ready.
void serial_tx_start(void) {
	// tx_state doesn't mean it's started but requested.
	tx_state = true;

	// Start tx only if the remote end is ready to receive.
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
		counts.too_long++;
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
		counts.too_long = 0;
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
	uint8_t out = serial_tx[serial_tx_i];

	if (out == '\0') {
		// Now it's the time to send last character.
		UCSR0B &= ~_BV(UDRIE0); // Disable this interrupt
		UDR0 = '\n'; // Send newline instead of NUL
	} else {
		// Otherwise transmit
		UDR0 = out;
		serial_tx_i++;
	}
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
