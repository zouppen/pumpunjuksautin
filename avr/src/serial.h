#pragma once

// Binary safe RS-485 interface

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t buflen_t;

// Serial buffer lengths. If you want to go beyond 255, remember to
// change buflen_t from uint8_t to uint16_t.
#define SERIAL_RX_LEN 80
#define SERIAL_TX_LEN 80

// Useful return value. Use < SERIAL_TX_LEN in comparison instead of
// this because this indicates the maximum value.
#define BUFLEN_MAX ({ buflen_t _a = ~0; _a; })

typedef struct {
	int good;         // Number of good frames
	int flip_timeout; // Number of missed flips
	int too_long_rx;  // Number of too long frames in receive
	int too_long_tx;  // Number of too long frames in transmit
} serial_counter_t;

// Serial buffer which is sent on invocation of serial_tx_start().
extern char serial_tx[SERIAL_TX_LEN];

// Initialize serial port.
void serial_init(void);

// Is serial transmitter on?
bool serial_is_transmitting(void);

// Gets a message from serial receive buffer, if any. The returned
// buffer is immutable. Buffer must be released after processing with
// serial_free_message(). In case buffer overflow, returns ~0.
buflen_t serial_get_message(char **const buf);

// Release receive buffer. This is important to do as soon as
// possible. If frame is not freed before back buffer is filled,
// error_flip_timeout occurs.
void serial_free_message(void);

// Start half-duplex transmission (disables rx). For sending NUL
// terminated strings. Outputs newline after every send.
void serial_tx_line(void);

// Start half-duplex transmission (disables rx). Binary safe.
void serial_tx_bin(buflen_t const len);

// Get serial counters and zero them
serial_counter_t pull_serial_counters(void);
