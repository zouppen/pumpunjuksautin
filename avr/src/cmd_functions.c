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

#include <time.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "cmd.h"
#include "pin.h"
#include "hardware_config.h"
#include "clock.h"

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

static buflen_t expected_count;

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
		return false;
	}
	// Pass it on to Modbus handler.
	return writer(&truth, 1) == 1;
}

// Outputs boolean value.
buflen_t cmd_print_bool(char *const buf_out, buflen_t count)
{
	ENSURE_COUNT(1)

	char val = *buf_out;
	*buf_out = val ? '1' : '0';
	return 1;
}

// Reads current time to 2 16-bit registers
buflen_t cmd_read_time(char *const buf_out, buflen_t count)
{
	ENSURE_COUNT(4);

	// Output time in big-endian byte order
	*(uint32_t*)buf_out = bswap_32(time(NULL));
	return 4;
}

// Writes current time. Consumes 2 16-bit registers
buflen_t cmd_write_time(char const *const buf_in, buflen_t count)
{
	ENSURE_COUNT(4);

	time_t now = bswap_32(*(uint32_t*)buf_in);
	clock_set_time(now);
	return 4;
}

// Writes time zone information.
buflen_t cmd_write_time_zone(char const *const buf_in, buflen_t count)
{
	ENSURE_COUNT(12);

	int32_t zone_now = bswap_32(*(int32_t*)buf_in); 
	time_t ts_turn = bswap_32(*(uint32_t*)(buf_in + 4));
	int32_t zone_turn = bswap_32(*(int32_t*)(buf_in + 8));

	clock_set_zones(zone_now, ts_turn, zone_turn);
	return 12;
}

// Formats big-endian time in ISO 8601 format with time zone
buflen_t cmd_print_time(char *const buf_out, buflen_t count)
{
	ENSURE_COUNT(4);

	time_t now = bswap_32(*(uint32_t*)buf_out);
	size_t wrote = strftime(buf_out, count, "%FT%T%z", localtime(&now));
	return wrote ? wrote-1 : OUT_OF_BUFFER;
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
