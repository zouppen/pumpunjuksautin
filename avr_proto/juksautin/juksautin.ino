// Pumpunjuksautin prototype for ATMega328p

// Note, many macro values are defined in <avr/io.h> and
// <avr/interrupts.h>, which are included automatically by
// the Arduino interface

#include <util/atomic.h>
#include "pin.h"

// Store analog measurement sum and measurement count. Used for mean
// calculation.
typedef struct accu_t {
	uint32_t sum;
	uint16_t count;
};

// Struct of accumulators.
typedef struct accus_t {
	accu_t k9_raw;    // Real voltage in K9
	accu_t int_temp;  // AVR internal temperature
	accu_t juksautin; // Juksautin ratio
};

#define VOLT (1.1f / 1024) // 1.1V AREF and 10-bit accuracy
#define PIN_FB C,1
#define SERIAL_BUF_LEN 40

volatile accus_t v_accu; // Holds all volatile measurement data
volatile uint16_t target; // Target voltage for juksautus
char serial_buf[SERIAL_BUF_LEN]; // Outgoing serial data
volatile int serial_tx_i = 0; // Send buffer position

// Set ADC source. Do not set above 15 because then you will overrun
// other parts of ADMUX. A full list of possible inputs is available
// in Table 24-4 of the ATMega328 datasheet.
void start_adc_sourcing(uint8_t chan) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// Clear MUX3..0 in ADMUX (0x7C) in preparation for setting the analog
		// input
		ADMUX &= B11110000;

		// Set MUX3..0 in ADMUX (0x7C) to select ADC input
		ADMUX |= chan;

		// Set ADSC in ADCSRA (0x7A) to start the ADC conversion
		ADCSRA |= B01000000;
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
void setup() {
	// Serial port setup
	Serial.begin(9600);
	while (!Serial) {
		// wait for serial port to connect. Needed for native USB port only
	}

	UCSR0B |= (1<<TXCIE0); // transmit ready interrupt

	set_voltage(0.5);

	// ADC setup

	// clear ADLAR in ADMUX (0x7C) to right-adjust the result
	// ADCL will contain lower 8 bits, ADCH upper 2 (in last two bits)
	ADMUX &= B11011111;

	// Set REFS1..0 in ADMUX (0x7C) to change reference voltage to internal
	// 1.1V reference
	ADMUX |= B11000000;

	// Set ADEN in ADCSRA (0x7A) to enable the ADC.
	// Note, this instruction takes 12 ADC clocks to execute
	ADCSRA |= B10000000;

	// Clear ADTS2..0 in ADCSRB (0x7B) to set trigger mode to free running.
	// This means that as soon as an ADC has finished, the next will be
	// immediately started.
	ADCSRB &= B11111000;

	// Set the Prescaler to 128 (16000KHz/128 = 125KHz)
	// Above 200KHz 10-bit results are not reliable.
	ADCSRA |= B00000111;

	// Set ADIE in ADCSRA (0x7A) to enable the ADC interrupt.
	// Without this, the internal interrupt will not trigger.
	ADCSRA |= B00001000;

	// Enable global interrupts
	// AVR macro included in <avr/interrupts.h>, which the Arduino IDE
	// supplies by default.
	sei();

	// Set ADSC in ADCSRA (0x7A) to start the ADC conversion
	start_adc_sourcing(8);

	// Set FB pin as input
	LOW(PIN_FB);
	INPUT(PIN_FB);
}

// Take analog accumulator mean value
float accu_mean(accu_t *a) {
	return (float)a->sum / a->count;
}

// Processor loop
void loop() {
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
		// front/back buffering at the moment
		memcpy(&accu, &v_accu, sizeof(accu));
		memset(&v_accu, 0, sizeof(v_accu));
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
	serial_flush();

	// Quick hack
	if (Serial.available() > 0) {
		float target = Serial.parseFloat();
		while (Serial.read() != '\n');
		set_voltage(target);
	}

	// Whatever else you would normally have running in loop().
}

// Interrupt service routine for the ADC completion
ISR(ADC_vect) {
	static bool juksautus = false;
	// Store the ADC port of previous measurement before changing it
	uint8_t port = ADMUX & B00001111;

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
		juksautus ? OUTPUT(PIN_FB) : INPUT(PIN_FB);

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
inline void store(accu_t *a, uint16_t val) {
	// Stop storing when full
	if (a->count == ~0) return;
	a->sum += val;
	a->count++;
}

void serial_flush(void) {
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
