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
#include <avr/eeprom.h>
#include "cmd.h"
#include "pin.h"
#include "hardware_config.h"
#include "clock.h"
#include "juksautin.h"

#define OUT_OF_BUFFER (~0)

// Using GCC builtins for byte order swaps
// https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
#define bswap_16(x) __builtin_bswap16(x)
#define bswap_32(x) __builtin_bswap32(x)
#define bswap_64(x) __builtin_bswap64(x)

#define ENSURE_COUNT(x) { expected_count = x; if (count < x) return OUT_OF_BUFFER; }

// Functions are used by external commands via Modbus or ASCII command
// interface, defined in file avr/commands.tsv.
//
// The prefix "cmd_" is added automatically to the function names in
// the table. So, "write_led" in the table maps to "cmd_write_led" in
// this object.
//
// Functions used in the table in commands.tsv do not require
// prototypes because those are auto-generated by `commands` script.

// Externs of these are exceptionally defined in cmd.h
char const *volatile cmd_parse_error;
volatile buflen_t cmd_parse_error_pos;
uint32_t cmd_parse_error_arg;
static buflen_t expected_count;

// Version definition is delivered by version.cmake
extern char const version[] PROGMEM;

// Changes indicator LED status. Consumes 1 byte.
buflen_t cmd_write_led(char const *const buf_in, buflen_t count)
{
	ENSURE_COUNT(1);

	if (*buf_in) {
		HIGH(PIN_LED);
	} else {
		LOW(PIN_LED);
	}
	return 1;
}

// Reads LED status, 1 byte.
buflen_t cmd_read_led(char *const buf_out, buflen_t count)
{	
	ENSURE_COUNT(1)

	*buf_out = STATE(PIN_LED);
	return 1;
}

// Scans input for boolean values. Accepting true, false, 0, and 1.
bool cmd_scan_bool(char const *const buf_in, cmd_write_t *writer)
{
	char truth;
	if (strcasecmp_P(buf_in, PSTR("on")) == 0) {
		truth = 1;
	} else if (strcasecmp_P(buf_in, PSTR("off")) == 0) {
		truth = 0;
	} else if (strcmp_P(buf_in, PSTR("1")) == 0) {
		truth = 1;
	} else if (strcmp_P(buf_in, PSTR("0")) == 0) {
		truth = 0;
	} else {
		// Parsing failed
		cmd_parse_error = PSTR("Allowed values: 0, 1, ON, or OFF");
		return false;
	}
	// Pass it on to Modbus handler.
	return writer(&truth, 1) == 1;
}

// Scans input for a single 16 bit integer, signed or unsigned.
bool cmd_scan_int16(char *const buf_in, cmd_write_t *writer)
{
	int read = 0;
	int16_t val;
	int items = sscanf_P(buf_in, PSTR("%" SCNi16 "%n"), &val, &read);
	if (items != 1 || buf_in[read] != '\0') {
		cmd_parse_error = PSTR("Not a digit");
		cmd_parse_error_pos = read;
		return false;
	}

	// Pass it on to Modbus handler.
	buflen_t wrote = writer((char*)&val, sizeof(val));
	if (wrote == sizeof(val)) {
		return true;
	}

	// Nice errors are being generated
	cmd_parse_error = PSTR("Data length mismatch. %d octets wanted");
	cmd_parse_error_arg = expected_count;
	return false;
}

// Scans input for a single 32 bit integer, signed or unsigned.
bool cmd_scan_int32(char *const buf_in, cmd_write_t *writer)
{
	int read = 0;
	int32_t val;
	int items = sscanf_P(buf_in, PSTR("%" SCNi32 "%n"), &val, &read);
	if (items != 1 || buf_in[read] != '\0') {
		cmd_parse_error = PSTR("Not a digit");
		cmd_parse_error_pos = read;
		return false;
	}

	// Pass it on to Modbus handler.
	buflen_t wrote = writer((char*)&val, sizeof(val));
	if (wrote == sizeof(val)) {
		return true;
	}

	// Nice errors are being generated
	cmd_parse_error = PSTR("Data length mismatch. %d octets wanted");
	cmd_parse_error_arg = expected_count;
	return false;
}

// Outputs boolean value.
buflen_t cmd_print_bool(char *const buf_out, buflen_t count)
{
	ENSURE_COUNT(1)

	char val = *buf_out;
	*buf_out = val ? '1' : '0';
	return 1;
}

// Outputs version number. Doesn't use buffer for input.
buflen_t cmd_print_version(char *const buf_out, buflen_t count)
{
	return strlcpy_P(buf_out, version, count);
}

// Reads current juksautus target to 1 16-bit register
buflen_t cmd_read_target(char *const buf_out, buflen_t count)
{
	ENSURE_COUNT(2);

	*(uint16_t*)buf_out = juksautin_get_target();
	return 2;
}

// Writes juksautus target voltage. Consumes 1 16-bit register
buflen_t cmd_write_target(char const *const buf_in, buflen_t count)
{
	ENSURE_COUNT(2);

	uint16_t a = *(uint16_t*)buf_in;
	juksautin_set_target(a);
	return 2;
}

// Reads K5 thermistor voltage in mV to 1 16-bit register
buflen_t cmd_read_k5_raw(char *const buf_out, buflen_t count)
{
	ENSURE_COUNT(2);

	*(uint16_t*)buf_out = juksautin_take_k5_raw_mv();
	return 2;
}

// Reads accumulator tank temperature to 1 16-bit register
buflen_t cmd_read_accu(char *const buf_out, buflen_t count)
{
	ENSURE_COUNT(2);

	*(uint16_t*)buf_out = juksautin_take_accumulator_temp();
	return 2;
}

// Reads outside temperature to 1 16-bit register
buflen_t cmd_read_out(char *const buf_out, buflen_t count)
{
	ENSURE_COUNT(2);

	*(uint16_t*)buf_out = juksautin_take_outside_temp();
	return 2;
}

// Reads error LED status to 1 16-bit register
buflen_t cmd_read_error(char *const buf_out, buflen_t count)
{
	ENSURE_COUNT(2);

	*(uint16_t*)buf_out = juksautin_take_error();
	return 2;
}

// Reads current time to 2 16-bit registers
buflen_t cmd_read_time(char *const buf_out, buflen_t count)
{
	ENSURE_COUNT(4);

	*(uint32_t*)buf_out = time(NULL) + UNIX_OFFSET;
	return 4;
}

// Writes current time. Consumes 2 16-bit registers
buflen_t cmd_write_time(char const *const buf_in, buflen_t count)
{
	ENSURE_COUNT(4);

	time_t now = *(uint32_t*)buf_in;
	clock_set_time(now);
	return 4;
}

// Writes current time zone
buflen_t cmd_write_gmtoff(char const *const buf_in, buflen_t count)
{
	ENSURE_COUNT(4);

	uint32_t a = *(uint32_t*)buf_in;
	eeprom_update_dword(&clock_ee_zone_now, a);
	clock_set_zones_from_eeprom();
	return 4;
}

// Writes next clock turn timestamp
buflen_t cmd_write_next_turn(char const *const buf_in, buflen_t count)
{
	ENSURE_COUNT(4);

	uint32_t a = *(uint32_t*)buf_in;
	eeprom_update_dword(&clock_ee_ts_turn, a);
	clock_set_zones_from_eeprom();
	return 4;
}

// Writes gmtoff after next clock turn.
buflen_t cmd_write_gmtoff_turn(char const *const buf_in, buflen_t count)
{
	ENSURE_COUNT(4);

	uint32_t a = *(uint32_t*)buf_in;
	eeprom_update_dword(&clock_ee_zone_turn, a);
	clock_set_zones_from_eeprom();
	return 4;
}

// Read currently observed UTC offset (time zone with DST adjustment,
// if any)
buflen_t cmd_read_gmtoff(char *const buf_out, buflen_t count)
{
	ENSURE_COUNT(4);

	*(int32_t*)buf_out = clock_get_gmtoff();
	return 4;
}

// Read next clock turn timestamp
buflen_t cmd_read_next_turn(char *const buf_out, buflen_t count)
{
	ENSURE_COUNT(4);

	*(int32_t*)buf_out = eeprom_read_dword(&clock_ee_ts_turn);
	return 4;
}

// Read next clock turn timestamp
buflen_t cmd_read_gmtoff_turn(char *const buf_out, buflen_t count)
{
	ENSURE_COUNT(4);

	*(int32_t*)buf_out = eeprom_read_dword(&clock_ee_zone_turn);
	return 4;
}

// Prints 16-bit signed integer in decimal format
buflen_t cmd_print_int16(char *const buf_out, buflen_t count)
{
	ENSURE_COUNT(2);

	int32_t val = *(uint16_t*)buf_out;
	return snprintf_P(buf_out, count, PSTR("%" PRId16), val);
}

// Prints 32-bit signed integer in decimal format
buflen_t cmd_print_int32(char *const buf_out, buflen_t count)
{
	ENSURE_COUNT(4);

	int32_t val = *(uint32_t*)buf_out;
	return snprintf_P(buf_out, count, PSTR("%" PRId32), val);
}

// Formats time in UNIX epoch in ISO 8601 format with time zone
buflen_t cmd_print_time(char *const buf_out, buflen_t count)
{
	ENSURE_COUNT(4);

	time_t now = *(uint32_t*)buf_out - UNIX_OFFSET;
	size_t wrote = strftime(buf_out, count, "%FT%T%z", localtime(&now));
	return wrote == 25 ? wrote-1 : OUT_OF_BUFFER;
}

// Just writes PONG. For testing
buflen_t cmd_print_pong(char *const buf_out, buflen_t count)
{
	ENSURE_COUNT(4);

	buf_out[0] = 'P';
	buf_out[1] = 'O';
	buf_out[2] = 'N';
	buf_out[3] = 'G';
	return 4;
}
