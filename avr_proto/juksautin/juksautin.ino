#include <util/atomic.h>

// Testing interrupt-based analog reading
// ATMega328p

// Note, many macro values are defined in <avr/io.h> and
// <avr/interrupts.h>, which are included automatically by
// the Arduino interface

// Value to store analog result
volatile uint16_t ana[16];
volatile uint16_t target;
volatile uint16_t meas_count = 0;
volatile uint16_t pulldowns = 0;

void start_adc_sourcing(uint8_t chan) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    // Clear MUX3..0 in ADMUX (0x7C) in preparation for setting the analog
    // input
    ADMUX &= B11110000;
   
    // Set MUX3..0 in ADMUX (0x7C) to read from AD8 (Internal temp)
    // Do not set above 15! You will overrun other parts of ADMUX. A full
    // list of possible inputs is available in Table 24-4 of the ATMega328
    // datasheet
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
void setup(){
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
 
  // Set ADATE in ADCSRA (0x7A) to enable auto-triggering.
  //ADCSRA |= B00100000;
 
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


// Processor loop
void loop(){
  // Check to see if the value has been updated
  uint16_t i,temp, volts, f, pd;
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    i = meas_count;
    temp = ana[8];
    volts = ana[0];
    pd = pulldowns;
  }
  
  Serial.print(i);
  Serial.print(' ');
  Serial.print(temp);
  Serial.print(' ');
  Serial.print(volts);
  Serial.print(' ');
  Serial.print(target);
  Serial.print(' ');
  Serial.print(pd);
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
ISR(ADC_vect){
  // Which port is in use
  int port = ADMUX & B00001111;

  // Must read low first
  uint16_t val = ADCL | (ADCH << 8);

  // Update analog value
  ana[port] = val;
  meas_count++;

  // Rotate between different analog inputs
  uint8_t cycle = meas_count & 0b111; // Cycle length: 8
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

  // Pump logic. Pull capacitor down to target voltage.
  if (port == 0) {
    bool pulldown  = val > target;
    pinMode(A1, pulldown ? OUTPUT : INPUT);
    pulldowns++;
  }
}
