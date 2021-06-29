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

char serial_tx[SERIAL_TX_LEN]; // Outgoing serial data

// Double buffering for rx
static char serial_rx_a[SERIAL_RX_LEN]; // Receive buffer a
static char serial_rx_b[SERIAL_RX_LEN]; // Receive buffer b
static char *serial_rx_back = serial_rx_a; // Back buffer (for populating data)

static int serial_rx_i = 0; // Receive buffer position
static int serial_tx_i = 0; // Send buffer position
static volatile bool may_flip = false; // Back buffer has a frame
static volatile bool rx_idle = true; // Is the remote end having a pause
static volatile bool tx_state = false; // Is tx start requested

// Static prototypes
static void transmit_now(void);

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

// Many RS-485 transceivers are not doing tx/rx handover quickly
// enough. Make sure the receiver has been idle before starting.
void serial_tx_start(void) {
	// Serial has to be idle before sending, so start only RX has
	// been silent.
	tx_state = true;
	if (rx_idle) transmit_now();
}

// Callback when it's safe to start transmitting
ISR(TIMER2_COMPB_vect) {
	// Timer is oneshot, cancel timer
	TIMSK2 &= ~_BV(OCIE2B);

	// RX is idle but start transmit only if it has been
	// requested.
	rx_idle = true;
	if (tx_state) transmit_now();
}

void transmit_now()
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

// Pulls message if any. The returned buffer is immutable.
char const *serial_pull_message(void)
{
	char *ret;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (may_flip) {
			may_flip = false;
			// Do back buffer flip and return the old back buffer.
			ret = serial_rx_back;
			serial_rx_back = (serial_rx_a == serial_rx_back)
				? serial_rx_b
				: serial_rx_a;
		} else {
			// Do not flip.
			ret = NULL;
		}
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
	char out = serial_tx[serial_tx_i];

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
	// Make the timeout longer if remote end doesn't do rx/tx
	// switch quickly enough.
	clock_arm_timer(20);
	rx_idle = false;

	char in = UDR0;
	bool fail = serial_rx_i == SERIAL_RX_LEN;
	bool end = in == '\n';
	may_flip = false;
	if (fail) {
		// Too long frame is completely ignored. When we have
		// newline, then rollback the buffer and prepare for a
		// new frame.
		if (end) {
			serial_rx_i = 0;
		}
	} else if (end) {
		// Ensure the frame is null-terminated
		serial_rx_back[serial_rx_i] = '\0';
		serial_rx_i = 0;

		// Ready to serve the frame.
		may_flip = true;
	} else {
		serial_rx_back[serial_rx_i] = in;
		serial_rx_i++;
	}
}
