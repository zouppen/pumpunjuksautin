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

#include "pin.h"
#include "serial.h"
#include "hardware_config.h"

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
void store(volatile accu_t *a, uint16_t val);
void loop(void);

#define VOLT (1.1f / 1024) // 1.1V AREF and 10-bit accuracy

volatile accus_t v_accu; // Holds all volatile measurement data
volatile uint16_t target; // Target voltage for juksautus

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
	OUTPUT(PIN_LED);
	
	serial_init();

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

	// Continue only if we have a new frame to parse-
	char const * const rx_buf = serial_pull_message();
	if (rx_buf == NULL) return;

	i++;

	// Parse command
	if (strcmp(rx_buf, "PING") == 0) {
		// Prepare ping answer
		strcpy(serial_tx, "PONG");
		serial_tx_start();
	} else if (strcmp(rx_buf, "LED") == 0) {
		// Useful for testing if rx works because we see
		// visual indication even if tx is bad.
		TOGGLE(PIN_LED);
	} else if (strcmp(rx_buf, "READ") == 0) {
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
void store(volatile accu_t *a, uint16_t val) {
	// Stop storing when full
	if (a->count == ~0) return;
	a->sum += val;
	a->count++;
}

