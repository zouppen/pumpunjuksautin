// Pumpunjuksautin prototype for ATMega328p

// Note, many macro values are defined in <avr/io.h> and
// <avr/interrupts.h>, which are included automatically by
// the Arduino interface

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <util/atomic.h>
#include <avr/io.h>
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

#define VOLT (1.1f / 1024) // 1.1V AREF and 10-bit accuracy
#define PIN_FB C,1
#define SERIAL_BUF_LEN 40

volatile accus_t v_accu; // Holds all volatile measurement data
volatile uint16_t target; // Target voltage for juksautus
char serial_buf[SERIAL_BUF_LEN]; // Outgoing serial data
volatile int serial_tx_i = 0; // Send buffer position

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
	UCSR0B |= _BV(TXCIE0); // Transmit ready interrupt
	UCSR0B |= _BV(RXEN0);  // Receive enable
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
	int writing;
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		writing = serial_tx_i;
	}
	if (writing) return;
	i++;

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

	int wrote = snprintf(serial_buf, SERIAL_BUF_LEN, "%" PRIu16 ": %d°C %dmV %d%% %" PRIu16 "\n", i, (int)int_temp, (int)(k9_raw*1000), (int)(ratio*100), accu.juksautin.count);
	
	if (wrote >= SERIAL_BUF_LEN) {
		// Ensuring endline in the end
		serial_buf[SERIAL_BUF_LEN-2] = '\n';
		serial_buf[SERIAL_BUF_LEN-1] = '\0';
	}
	serial_tx_start();

	// Quick hack
	// TODO serial rx

	// Whatever else you would normally have running in loop().
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
		UDR0 = serial_buf[0];
		serial_tx_i = 1;
	}
}

ISR(USART_TX_vect)
{
	// Don't go further if it has been already sent
	if (serial_tx_i == 0) return;

	char out = serial_buf[serial_tx_i];
	UDR0 = out;

	// Stop at NUL char
	if (out == '\0') {
		serial_tx_i = 0;
	} else {
		serial_tx_i++;
	}
}
