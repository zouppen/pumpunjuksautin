// Pumpunjuksautin interface functions.
//
// SPDX-License-Identifier:   GPL-3.0-or-later
// Copyright (C) 2021 Joel Lehtonen
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "cmd.h"
#include "pin.h"
#include "hardware_config.h"

#define OUT_OF_BUFFER (~0)

const cmd_result_t cmd_scan_success = { 0, NULL, 0 };

// Using GCC builtins for byte order swaps
// https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
#define bswap_16(x) __builtin_bswap16(x)
#define bswap_32(x) __builtin_bswap32(x)
#define bswap_64(x) __builtin_bswap64(x)

// Functions are used by external commands via Modbus or ASCII command
// interface, defined in file avr/commands.tsv.
//
// The prefix "cmd_" is added automatically to the function names in
// the table. So, "write_led" in the table maps to "cmd_write_led" in
// this object.
//
// Functions used in the table in commands.tsv do not require
// prototypes because those are auto-generated by `commands` script.

static char const *modbus_strerror(modbus_status_t e);

// Typedefs for supported getters & setters
typedef int16_t get_int16_t(void);
typedef int32_t get_int32_t(void);
typedef bool get_bool_t(void);
typedef modbus_status_t set_int16_t(int16_t);
typedef modbus_status_t set_int32_t(int32_t);
typedef modbus_status_t set_bool_t(bool);

// Version definition is delivered by version.cmake
extern char const version[] PROGMEM;

// Scans input for boolean values. Accepting true, false, 0, and 1.
cmd_result_t cmd_scan_bool(char *const buf_in, void *setter)
{
	bool val;
	if (strcasecmp_P(buf_in, PSTR("on")) == 0) {
		val = true;
	} else if (strcasecmp_P(buf_in, PSTR("off")) == 0) {
		val = false;
	} else if (strcmp_P(buf_in, PSTR("1")) == 0) {
		val = true;
	} else if (strcmp_P(buf_in, PSTR("0")) == 0) {
		val = false;
	} else {
		// Parsing failed
		const cmd_result_t e = {0, PSTR("Allowed values: 0, 1, ON, or OFF"), 0};
		return e;
	}

	// Pass it on setter
	set_bool_t *f = setter;
	modbus_status_t status = f(val);
	if (status == MODBUS_OK) {
		return cmd_scan_success;
	} else {
		const cmd_result_t e = {0, modbus_strerror(status), status};
		return e;
	}
}

// Scans input for a single 16 bit integer, signed or unsigned.
cmd_result_t cmd_scan_int16(char *const buf_in, void *setter)
{
	int read = 0;
	int16_t val;
	int items = sscanf_P(buf_in, PSTR("%" SCNi16 "%n"), &val, &read);
	if (items != 1 || buf_in[read] != '\0') {
		const cmd_result_t e = { read, PSTR("Not a digit"), 0};
		return e;
	}

	// Pass it on setter
	set_int16_t *f = setter;
	modbus_status_t status = f(val);
	if (status == MODBUS_OK) {
		return cmd_scan_success;
	} else {
		const cmd_result_t e = {0, modbus_strerror(status), status};
		return e;
	}
}

// Scans input for a single 32 bit integer, signed or unsigned.
cmd_result_t cmd_scan_int32(char *const buf_in, void *setter)
{
	int read = 0;
	int32_t val;
	int items = sscanf_P(buf_in, PSTR("%" SCNi32 "%n"), &val, &read);
	if (items != 1 || buf_in[read] != '\0') {
		const cmd_result_t e = { read, PSTR("Not a digit"), 0};
		return e;
	}

	// Pass it on setter
	set_int32_t *f = setter;
	modbus_status_t status = f(val);
	if (status == MODBUS_OK) {
		return cmd_scan_success;
	} else {
		const cmd_result_t e = {0, modbus_strerror(status), status};
		return e;
	}
}

// Outputs boolean value.
buflen_t cmd_print_bool(char *const buf_out, buflen_t count, void *getter)
{
	if (count < 1) return OUT_OF_BUFFER;

	get_bool_t *f = getter;
	*buf_out = f() ? '1' : '0';
	return 1;
}

// Outputs version number. Doesn't use buffer for input.
buflen_t cmd_print_version(char *const buf_out, buflen_t count, void *_)
{
	return strlcpy_P(buf_out, version, count);
}

// Gets and prints 16-bit signed integer in decimal format
buflen_t cmd_print_int16(char *const buf_out, buflen_t count, void *getter)
{
	get_int16_t *f = getter;
	return snprintf_P(buf_out, count, PSTR("%" PRId16), f());
}

// Gets and prints 32-bit signed integer in decimal format.
buflen_t cmd_print_int32(char *const buf_out, buflen_t count, void *getter)
{
	get_int32_t *f = getter;
	return snprintf_P(buf_out, count, PSTR("%" PRId32), f());
}

// Formats current time in ISO 8601 format with time zone
buflen_t cmd_print_now(char *const buf_out, buflen_t count, void *_)
{
	time_t now = time(NULL);
	size_t wrote = strftime(buf_out, count, "%FT%T%z", localtime(&now));
	return wrote == 25 ? wrote-1 : OUT_OF_BUFFER;
}

static char const *modbus_strerror(modbus_status_t e)
{
	switch (e) {
	case MODBUS_OK: return NULL; // All is fine
	case MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE: return PSTR("Illegal data value");
	case MODBUS_EXCEPTION_SLAVE_OR_SERVER_FAILURE: return PSTR("Unrecoverable error");
	default: return PSTR("Modbus error %d");
	}
}

// Just writes PONG. For testing
buflen_t cmd_print_pong(char *const buf_out, buflen_t count, void *_)
{
	if (count < 4) return OUT_OF_BUFFER;

	buf_out[0] = 'P';
	buf_out[1] = 'O';
	buf_out[2] = 'N';
	buf_out[3] = 'G';
	return 4;
}
