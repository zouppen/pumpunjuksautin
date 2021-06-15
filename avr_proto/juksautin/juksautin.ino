#include <util/atomic.h>

// Pumpunjuksautin prototype for ATMega328p

// Note, many macro values are defined in <avr/io.h> and
// <avr/interrupts.h>, which are included automatically by
// the Arduino interface

// Store analog measurement sum and measurement count. Used for mean
// calculation.
typedef struct accu_t {
	uint32_t sum;
	uint16_t count;
};

#define K9_RAW (0)    // Real voltage in K9
#define INT_TEMP (1)  // AVR internal temperature
#define JUKSAUTIN (2) // Juksautin ratio
#define VOLT (1.1f / 1024) // 1.1V AREF and 10-bit accuracy

volatile accu_t v_accu[3];
volatile uint16_t target; // Target voltage for juksautus

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

	pinMode(A1, OUTPUT);
	digitalWrite(A1, LOW);
}

// Take analog accumulator mean value
float accu_mean(accu_t *a) {
	return (float)a->sum / a->count;
}

// Processor loop
void loop() {
	static uint16_t i = ~0;
	i++;

	// Duplicate the data
	accu_t accu[3];
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		// The buffer is so small it makes more sense to use copying than
		// front/back buffering at the moment
		memcpy(&accu, &v_accu, sizeof(accu));
		memset(&v_accu, 0, sizeof(v_accu));
	}

	float ratio = accu_mean(&accu[JUKSAUTIN]);
	float int_temp = (accu_mean(&accu[INT_TEMP])-324.31)/1.22;
	float k9_raw = accu_mean(&accu[K9_RAW]) * VOLT;

	Serial.print(i);
	Serial.print(": ");
	Serial.print(int_temp);
	Serial.print("°C ");
	Serial.print(k9_raw);
	Serial.print("V ");
	Serial.print(ratio*100);
	Serial.print("% ");
	Serial.print(accu[JUKSAUTIN].count);
	Serial.print('\n');

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
	uint8_t cycle = v_accu[JUKSAUTIN].count & 0b111; // Cycle length: 8
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
		pinMode(A1, juksautus ? OUTPUT : INPUT);

		// Store measurement
		store(&v_accu[K9_RAW], val);

		break;
	case 8:
		store(&v_accu[INT_TEMP], val);
		break;
	}

	// What we really are interested is the ratio of pulldowns and total measurements.
	// That allows us to calculate the real thermistor value while juksautus is happening.
	// We calculate juksautus count by counting the periods of time the current is flowing.
	store(&v_accu[JUKSAUTIN], juksautus);
}

// Update cumulative analog value for access outside the ISR. Do not let it overflow.
inline void store(accu_t *a, uint16_t val) {
	// Stop storing when full
	if (a->count == ~0) return;
	a->sum += val;
	a->count++;
}
