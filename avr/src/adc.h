#pragma once

// Analog-to-digital interface

// Function which processes incoming ADC data.
typedef void (*adc_handler_t)(uint16_t val);

// Initialize ADC.
void adc_init(void);

// Set ADC result handler. Even though this is called with interrupts
// enabled, it is called within an interrupt so it must return before
// the next ADC value comes out.
void adc_set_handler(uint8_t channel, adc_handler_t func);

// Start digitizing given channel.
void adc_start_sourcing(uint8_t chan);

// You need to implement this. Selects ADC channel based on your
// criteria.
uint8_t adc_channel_selection(void);
