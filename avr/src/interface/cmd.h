#pragma once

// Command interface data types

#include "../serial.h"
#include "../modbus_types.h"

// Scan result. If error_msg == NULL it succeeded
typedef struct {
	buflen_t error_pos;    // Position relative to buf_in
	char const *error_msg; // Error, if any. Stored in PROGMEM.
	uint32_t msg_arg1;     // First argument to the error msg
} cmd_result_t;

// Scan result. If error_msg == NULL it succeeded
typedef struct {
	modbus_status_t code;  // Error code or MODBUS_OK
	buflen_t consumed;     // Bytes consumed
} cmd_modbus_result_t;

// Successful scan result. Defined in cmd_functions.c
extern const cmd_result_t cmd_success;

// Function which parses already tokenized (null terminated) text and
// passes it to given setter function. Returns cmd_result_t
// containing scanning result. Parameter buf_in is not const because
// further tokenization might be needed in the function.
typedef cmd_result_t cmd_scan_t(char *const buf_in, void const *setter);

// Function for ASCII output for humans. It takes in the buffer where
// to write the output, remaining buffer length and getter function
// where to receive the value to output.
typedef buflen_t cmd_print_t(char *const buf_out, buflen_t count, void const *getter);

// Modbus read command. It runs given getter function and produces
// binary output with big endian byte order (mandated by Modbus).
typedef cmd_modbus_result_t cmd_bin_read_t(char *const buf_out, buflen_t count, void const *getter);

// Modbus write command. It retrieves the data from input buffer,
// changes endianness from big endian and passes the data to given
// setter function.
typedef cmd_modbus_result_t cmd_bin_write_t(char const *const buf_in, buflen_t count, void const *setter);

// Aliases. Scanners use scanf() which can parse unsigned numbers
// correctly, no need to implement them twice.
#define cmd_scan_uint16 cmd_scan_int16
#define cmd_scan_uint32 cmd_scan_int32

// We really would need unsigned printers separately, but TODO
#define cmd_print_uint16 cmd_print_int16
#define cmd_print_uint32 cmd_print_int32

// Modbus aliases. In there signed and unsigned are the same basically.
#define cmd_bin_read_uint16 cmd_bin_read_int16
#define cmd_bin_read_uint32 cmd_bin_read_int32
#define cmd_bin_write_uint16 cmd_bin_write_int16
#define cmd_bin_write_uint32 cmd_bin_write_int32

typedef struct {
	void *read;           // NULL if not readable
	void *write;          // NULL if not writable
} cmd_action_t;

typedef struct {
	char const *name;           // ASCII command name, PROGMEM storage
	cmd_action_t const *action; // Read and write actions
	cmd_print_t *printer;       // Output formatter from raw value to ASCII. NULL if not readable.
	cmd_scan_t *scanner;        // Input scanner from ASCII to raw value. NULL if not writable.
} cmd_ascii_t;

typedef struct {
	modbus_object_t type;       // Object type, see definition
	uint16_t addr;              // Modbus address
	cmd_action_t const *action; // Getter and setter
	cmd_bin_read_t *reader;     // Modbus data reader
	cmd_bin_write_t* writer;    // Modbus data writer
} cmd_modbus_t;

// Typedefs for supported getters & setters
typedef int16_t get_int16_t(void);
typedef int32_t get_int32_t(void);
typedef uint16_t get_uint16_t(void);
typedef uint32_t get_uint32_t(void);
typedef bool get_bool_t(void);
typedef buflen_t get_string_t(char *, buflen_t);
typedef modbus_status_t set_int16_t(int16_t);
typedef modbus_status_t set_int32_t(int32_t);
typedef modbus_status_t set_uint16_t(uint16_t);
typedef modbus_status_t set_uint32_t(uint32_t);
typedef modbus_status_t set_bool_t(bool);

// ASCII command interface. Sorted by command name.
extern cmd_ascii_t const cmd_ascii[];
extern int const cmd_ascii_len;

// Modbus command interface. Sorted by (command type, address).
extern cmd_modbus_t const cmd_modbus[];
extern int const cmd_modbus_len;
