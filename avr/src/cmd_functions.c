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

#define OUT_OF_BUFFER (~0)

// Functions are used by external commands via Modbus or ASCII command
// interface.
//
// The non-static functions here are exceptional. They don't need
// prototypes because those are auto-generated by `commands` script.

// Changes indicator LED status. Consumes 1 byte.
buflen_t cmd_write_led(char const *const buf_in, buflen_t count) {
	if (count == 0) return 0;
	if (*buf_in) {
		HIGH(PIN_LED);
	} else {
		LOW(PIN_LED);
	}
	return 1;
}

// Scans input for boolean values. Accepting true, false, 0, and 1.
bool cmd_scan_bool(char const *const buf_in, cmd_write_t *writer) {
	char truth;
	if (strcasecmp_P(buf_in, PSTR("true")) == 0) {
		truth = 1;
	} else if (strcasecmp_P(buf_in, PSTR("false")) == 0) {
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
	return writer(&truth, 1);
}

// Reads current time to 2 16-bit registers
buflen_t cmd_read_clock(char *const buf_out, buflen_t count)
{
	// Has to have space for 2 16-bit registers.
	if (count < 4) return OUT_OF_BUFFER;

	// Output time in big-endian byte order
	uint32_t *dest = (uint32_t*)buf_out;
	*dest = __builtin_bswap32(time(NULL));
	return 4;
}

// Formats big-endian time in ISO 8601 format with time zone
buflen_t cmd_print_time(char *const buf_out, buflen_t count)
{
	if (count < 4) return OUT_OF_BUFFER;

	time_t now = __builtin_bswap32(*(uint32_t*)buf_out);
	size_t wrote = strftime(buf_out, count, "%F %T%z", localtime(&now));
	return wrote ? wrote-1 : OUT_OF_BUFFER;
}

// Just writes PONG. For testing
buflen_t cmd_print_pong(char *const buf_out, buflen_t count)
{
	if (count < 4) return OUT_OF_BUFFER;
	buf_out[0] = 'P';
	buf_out[1] = 'O';
	buf_out[2] = 'N';
	buf_out[3] = 'G';
	return 4;
}
