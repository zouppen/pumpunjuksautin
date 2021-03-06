#pragma once

#include "modbus_types.h"

// Functions specific to Pumpunjuksautin. NB! Functions with _take_ in
// the name read and empty the internal average counter.

// Initialize Juksautin ADC handlers and start with juksautus off
// (target voltage 1.0V).
void juksautin_init(void);

// Set given target voltage to K5 line in millivolts. The electronics
// can only do pulldown so it make voltage lower and therefore make
// the measured temperature higher (NTC thermistor)
modbus_status_t juksautin_set_target(uint16_t mv);

// Get previously set target voltage of K5 line in millivolts.
uint16_t juksautin_get_target(void);

// Calculate voltage in K5 line as if no juksautus was active. It is
// calculated from the measured voltage mv, juksautus ratio, thermal
// pump controller reference voltage um, and pull-up resistor
// rm. Units are in volts and ohms.
float juksautin_compute_k5_normal_voltage(float mv, float ratio, float um, float rm);

// Get current K5 temperature sensor value in millivolts.
uint16_t juksautin_take_k5_raw_mv(void);

// Get juksautin duty cycle in range 0-65535.
uint16_t juksautin_take_ratio(void);

// Get error LED high value. TODO: Should we return bool instead?
uint16_t juksautin_take_error(void);

// Get outside temperature
uint16_t juksautin_take_outside_temp(void);

// Get accumulator tank temperature
uint16_t juksautin_take_accumulator_temp(void);
