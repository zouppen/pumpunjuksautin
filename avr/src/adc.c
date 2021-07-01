#include <stdlib.h>
#include <avr/io.h>
#include <util/atomic.h>

#include "adc.h"

static adc_handler_t adc_handlers[9];

// Prototypes
static void call_handler(uint8_t chan, uint16_t val);

void adc_init(void)
{
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
}

void adc_set_handler(uint8_t channel, adc_handler_t func)
{
	adc_handlers[channel] = func;
}

// Set ADC source. Do not set above 15 because then you will overrun
// other parts of ADMUX. A full list of possible inputs is available
// in Table 24-4 of the ATMega328 datasheet.
void adc_start_sourcing(uint8_t chan)
{
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

static void call_handler(uint8_t chan, uint16_t val)
{
	if (chan >= sizeof(adc_handlers)/sizeof(adc_handler_t)) {
		// Out of bounds
		return;
	}
	if (adc_handlers[chan] == NULL) {
		// No handler registered
		return;
	}
	adc_handlers[chan](val);
}

// Interrupt service routine for the ADC completion
ISR(ADC_vect) {
	// Store the ADC port of previous measurement before changing it
	uint8_t port = ADMUX & 0b00001111;

	// Start the next measurement ASAP to keep ADC digitizing.
	adc_start_sourcing(adc_channel_selection());

	// Obtain previous result.
	uint16_t val = ADCW;

	// Enable interrupts while processing the data
	sei();
	call_handler(port, val);
}
