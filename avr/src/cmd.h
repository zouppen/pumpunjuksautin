#pragma once

// Command interface data types

#include "serial.h"
#include "modbus_types.h"

// Scan result. If error_msg == NULL it succeeded
typedef struct {
	buflen_t error_pos;    // Position relative to buf_in
	char const *error_msg; // Error, if any. Stored in PROGMEM.
	uint32_t msg_arg1;     // First argument to the error msg
} cmd_result_t;

// Successful scan result. Defined in cmd_functions.c
extern const cmd_result_t cmd_scan_success;

// Function which parses already tokenized (null terminated) text and
// passes it to given setter function. Returns cmd_result_t
// containing scanning result. Parameter buf_in is not const because
// further tokenization might be needed in the function.
typedef cmd_result_t cmd_scan_t(char *const buf_in, void const *setter);

// Function for ASCII output for humans. It takes in the buffer where
// to write the output, remaining buffer length and getter function
// where to receive the value to output.
typedef buflen_t cmd_print_t(char *const buf_out, buflen_t count, void const *getter);

typedef struct {
	void *read;           // NULL if not readable
	void *write;          // NULL if not writable
} cmd_action_t;

typedef struct {
	char const *name;     // ASCII command name, PROGMEM storage
	cmd_action_t action;  // Read and write actions
	cmd_print_t *printer; // Output formatter from raw value to ASCII. NULL if not readable.
	cmd_scan_t *scanner;  // Input scanner from ASCII to raw value. NULL if not writable.
} cmd_ascii_t;

typedef struct {
	modbus_object_t type; // Object type, see definition
	uint16_t addr;        // Modbus address
	cmd_action_t action;  // Read and write actions
} cmd_modbus_t;

// ASCII command interface. Sorted by command name.
extern cmd_ascii_t const cmd_ascii[];
extern int const cmd_ascii_len;

// Modbus command interface. Sorted by (command type, address).
extern cmd_modbus_t const cmd_modbus[];
extern int const cmd_modbus_len;

