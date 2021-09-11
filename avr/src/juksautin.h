#ifndef CMD_JUKSAUTIN_H_
#define CMD_JUKSAUTIN_H_

// Functions specific to Pumpunjuksautin

// Initialize Juksautin ADC handlers and start with juksautus off
// (target voltage 1.0V).
void juksautin_init(void);

// Set given target voltage to K9 line in millivolts. The electronics
// can only do pulldown so it make voltage lower and therefore make
// the measured temperature higher (NTC thermistor)
void juksautin_set_k9_target(uint16_t mv);

// Calculate voltage in K9 line as if no juksautus was active. It is
// calculated from the measured voltage mv, juksautus ratio, thermal
// pump controller reference voltage um, and pull-up resistor
// rm. Units are in volts and ohms.
float juksautin_compute_k9_normal_voltage(float mv, float ratio, float um, float rm);

// Get current K9 temperature sensor value in millivolts.
uint16_t juksautin_get_k9_raw_mv(void);

// Get juksautin duty cycle in range 0-65535.
uint16_t juksautin_get_ratio(void);

#endif // CMD_JUKSAUTIN_H_
