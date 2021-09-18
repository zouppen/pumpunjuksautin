#pragma once

// Command interface data types

#include "serial.h"
#include "modbus_types.h"

// Function pointer for read command. Populates buffer of given length
// and returns number of bytes written
typedef buflen_t (cmd_read_t)(char *const buf_out, buflen_t count);

// Function pointer for write command. Reads buffer of given length
// and return number of bytes processed.
typedef buflen_t (cmd_write_t)(char const *const buf_in, buflen_t count);

// Function which parses already tokenized (null terminated) text and
// passes it to given write function. Returns true if the scanning
// succeeded. Parameter buf_in is not const because further
// tokenization might be needed in the function.
typedef bool (cmd_scan_t)(char *buf_in, cmd_write_t *writer);

// Function for textual output. It takes in the buffer where reader
// has already written the values and it rewrites the contents to
// ASCII form.
typedef buflen_t (cmd_print_t)(char *const buf_out, buflen_t count);

typedef struct {
	cmd_read_t *read;      // NULL if not readable
	cmd_write_t *write;    // NULL if not writable
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

// For passing errors out of difficult places. Points to PROGMEM storage.
extern char const *volatile cmd_parse_error;

// Parse error position in the buffer. For nicer errors
extern volatile buflen_t cmd_parse_error_pos;

// Extra 4 bytes for formatted strings. TODO vararg stuff.
extern uint32_t cmd_parse_error_arg;
