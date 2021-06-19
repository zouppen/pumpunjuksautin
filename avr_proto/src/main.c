// Pumpunjuksautin prototype for ATMega328p

// Note, many macro values are defined in <avr/io.h> and
// <avr/interrupts.h>, which are included automatically by
// the Arduino interface

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <util/atomic.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/setbaud.h>
#include "pin.h"

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
	accu_t juksautin; // Juksautin ratio
} accus_t;

// Prototypes
inline void store(volatile accu_t *a, uint16_t val);
void serial_tx_start(void);
static void init_uart(void);
void loop(void);
void rx_toggle(void);
void tx_toggle(void);

#define VOLT (1.1f / 1024) // 1.1V AREF and 10-bit accuracy
#define PIN_FB C,1
#define PIN_TX_EN D,2
#define PIN_LED D,3
#define SERIAL_TX_LEN 40
#define SERIAL_RX_LEN 30

volatile accus_t v_accu; // Holds all volatile measurement data
volatile uint16_t target; // Target voltage for juksautus
char serial_tx[SERIAL_TX_LEN]; // Outgoing serial data
char serial_rx[SERIAL_RX_LEN]; // Incoming serial data
volatile int serial_tx_i = 0; // Send buffer position
volatile int serial_rx_i = 0; // Receive buffer position
volatile bool msg_available = false; // Message available in input buffer

void rx_toggle(void)
{
	UCSR0B ^= _BV(RXEN0);
}

void tx_toggle(void)
{
	TOGGLE(PIN_TX_EN);
	TOGGLE(PIN_LED);
}

static void init_uart(void)
{
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
#if USE_2X
	UCSR0A |= _BV(U2X0);
#else
	UCSR0A &= ~_BV(U2X0);
#endif

	// Set frame format to 8 data bits, no parity, 1 stop bit
	UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00);

	UCSR0B |= _BV(TXEN0);  // Tranmitter enabled
	// Transmitter interrupts are enabled later.
	rx_toggle();           // Receive enable
	UCSR0B |= _BV(RXCIE0); // Receive ready interrupt
}

// Set ADC source. Do not set above 15 because then you will overrun
// other parts of ADMUX. A full list of possible inputs is available
// in Table 24-4 of the ATMega328 datasheet.
void start_adc_sourcing(uint8_t chan) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// Clear MUX3..0 in ADMUX (0x7C) in preparation for setting the analog
		// input
		ADMUX &= 0b11110000;

		// Set MUX3..0 in ADMUX (0x7C) to select ADC input
		ADMUX |= chan;

		// Set ADSC in ADCSRA (0x7A) to start the ADC conversion
		ADCSRA |= 0b01000000;
	}
}

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
	OUTPUT(PIN_TX_EN);
	OUTPUT(PIN_LED);
	
	init_uart();

	set_voltage(0.5);

	// ADC setup

	// clear ADLAR in ADMUX (0x7C) to right-adjust the result
	// ADCL will contain lower 8 bits, ADCH upper 2 (in last two bits)
	ADMUX &= 0b11011111;

	// Set REFS1..0 in ADMUX (0x7C) to change reference voltage to internal
	// 1.1V reference
	ADMUX |= 0b11000000;

	// Set ADEN in ADCSRA (0x7A) to enable the ADC.
	// Note, this instruction takes 12 ADC clocks to execute
	ADCSRA |= 0b10000000;

	// Clear ADTS2..0 in ADCSRB (0x7B) to set trigger mode to free running.
	// This means that as soon as an ADC has finished, the next will be
	// immediately started.
	ADCSRB &= 0b11111000;

	// Set the Prescaler to 128 (16000KHz/128 = 125KHz)
	// Above 200KHz 10-bit results are not reliable.
	ADCSRA |= 0b00000111;

	// Set ADIE in ADCSRA (0x7A) to enable the ADC interrupt.
	// Without this, the internal interrupt will not trigger.
	ADCSRA |= 0b00001000;

	// Enable global interrupts
	// AVR macro included in <avr/interrupts.h>, which the Arduino IDE
	// supplies by default.
	sei();

	// Set ADSC in ADCSRA (0x7A) to start the ADC conversion
	start_adc_sourcing(8);

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

	// Come here only after serial comms is finished.
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		// Leave if nothing for us. This should be safe even
		// inside ATOMIC_BLOCK, see:
		// https://blog.oddbit.com/post/2019-02-01-atomicblock-magic-in-avrlibc/
		if (!msg_available) return;

		// OK, cleaning flag and turn off RX for a while to
		// make sure we don't mess the buffer. TODO double
		// buffering.
		msg_available = false;
		rx_toggle();
	}

	i++;

	// Parse command
	if (strcmp(serial_rx, "PING") == 0) {
		// Prepare ping answer
		strcpy(serial_tx, "PONG");
		serial_tx_start();
	} else if (strcmp(serial_rx, "READ") == 0) {
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
		float k9_raw = accu_mean(&accu.k9_raw) * VOLT;

		int wrote = snprintf(serial_tx, SERIAL_TX_LEN, "%" PRIu16 ": %d°C %dmV %d%% %" PRIu16, i, (int)int_temp, (int)(k9_raw*1000), (int)(ratio*100), accu.juksautin.count);
	
		if (wrote >= SERIAL_TX_LEN) {
			// Ensuring endline in the end
			serial_tx[SERIAL_TX_LEN-1] = '\0';
		}
		serial_tx_start();
	} else {
		// Do not answer to unrelated messages. This is
		// important to handle point-to-multipoint protocol:
		// We are not answering packets not related to us.
		rx_toggle(); // Turn receiver back on
	}
}

// Interrupt service routine for the ADC completion
ISR(ADC_vect) {
	static bool juksautus = false;
	// Store the ADC port of previous measurement before changing it
	uint8_t port = ADMUX & 0b00001111;

	// Start the next measurement ASAP to minimize drift caused by this ISR.
	// Rotate between different analog inputs.
	uint8_t cycle = v_accu.juksautin.count & 0b111; // Cycle length: 8
	switch (cycle) {
	case 0:
		// Internal temperature measurement
		start_adc_sourcing(8);
		break;
	default:
		// NTC thermistor measurement
		start_adc_sourcing(0);
		break;
	}

	// Obtain previous result.
	uint16_t val = ADCW;

	switch (port) {
	case 0:
		// Pump logic. Pull capacitor down to target voltage.
		juksautus = val > target;
		if (juksautus) {
			OUTPUT(PIN_FB);
		} else {
			INPUT(PIN_FB);
		}

		// Store measurement
		store(&v_accu.k9_raw, val);

		break;
	case 8:
		store(&v_accu.int_temp, val);
		break;
	}

	// What we really are interested is the ratio of pulldowns and total measurements.
	// That allows us to calculate the real thermistor value while juksautus is happening.
	// We calculate juksautus count by counting the periods of time the current is flowing.
	store(&v_accu.juksautin, juksautus);
}

// Update cumulative analog value for access outside the ISR. Do not let it overflow.
inline void store(volatile accu_t *a, uint16_t val) {
	// Stop storing when full
	if (a->count == ~0) return;
	a->sum += val;
	a->count++;
}

void serial_tx_start(void) {
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		tx_toggle();
		serial_tx_i = 0;
		UCSR0B |= _BV(UDRIE0); // Activate USART_UDRE_vect
	}
}

// Transmit ready. This interrupt is activated only for the last byte
// to handle RS-485 half-duplex handover.
ISR(USART_TX_vect)
{
	// TX off, RX on.
	tx_toggle();
	rx_toggle();

	// Disable this interrupt.
	UCSR0B &= ~_BV(TXCIE0);
}

// Called when TX buffer is empty.
ISR(USART_UDRE_vect)
{
	char out = serial_tx[serial_tx_i];

	if (out == '\0') {
		// Now it's the time to send last character.
		UCSR0B |= _BV(TXCIE0); // Enable TX_vect
		UCSR0B &= ~_BV(UDRIE0); // Disable this interrupt
		UDR0 = '\n'; // Send newline instead of NUL
	} else {
		// Otherwise transmit
		UDR0 = out;
		serial_tx_i++;
	}
}

ISR(USART_RX_vect)
{
	char in = UDR0;
	bool fail = serial_rx_i == SERIAL_RX_LEN;
	bool end = in == '\n';
	msg_available = false;
	if (fail) {
		if (end) {
			// Failure has NUL in the first byte
			serial_rx[0] = '\0';
			serial_rx_i = 0;
		} else {
			// Just ignore the byte until newline
		}
	} else if (end) {
		// All strings are null-terminated
		serial_rx[serial_rx_i] = '\0';
		serial_rx_i = 0;
		msg_available = true;
	} else {
		serial_rx[serial_rx_i] = in;
		serial_rx_i++;
	}
}
