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
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "serial.h"
#include "cmd.h"

#define SERIAL_TX_END (serial_tx + SERIAL_TX_LEN)

static bool strip_line_ending(char *buf, int const len);
static bool process_read(char *buf, char *serial_out);
static bool process_write(char *buf);
static cmd_ascii_t const *find_cmd(char const *const name);
static int cmd_comparator(const void *key_void, const void *item_void);

// Replace line ending (LF or CRLF) from the message with NUL
// character.
static bool strip_line_ending(char *const buf, int const len)
{
	// No LF is a showstopper
	if (buf[len-1] != '\n') return false;

	// Clean trailing CR and LF
	buf[len-1] = '\0';
	if (len >= 2 && buf[len-2] == '\r') {
		buf[len-2] = '\0';
	}

	return true;
}

// Process read requests, writing to serial buffer.
static bool process_read(char *buf, char *serial_out)
{
	// Collecting parameter name to read
	char const *const name = strsep(&buf, " ");
	cmd_ascii_t const *const cmd = find_cmd(name);

	if (cmd == NULL) {
		snprintf_P(serial_tx, SERIAL_TX_LEN, PSTR("Unknown field: \"%s\""), name);
		return false;
	}

	// Collecting scanner and writer from PROGMEM storage
	cmd_print_t const *printer = pgm_read_ptr_near(&(cmd->printer));
	cmd_write_t const *reader = pgm_read_ptr_near(&(cmd->action.read));

	if (printer == NULL) {
		char const *msg = reader == NULL
			? PSTR("Field \"%s\" is not readable")
			: PSTR("Field \"%s\" is readable via Modbus only!");

		snprintf_P(serial_tx, SERIAL_TX_LEN, msg, name);
		return false;
	}

	// Output variable name. Ensure space for an equals sign.
	serial_out += strlcpy(serial_out, name, SERIAL_TX_END - serial_out);
	if (serial_out+1 >= SERIAL_TX_END) goto serial_full;
	*serial_out++ = '=';
	
	// Collect the data first to the serial buffer. No NUL byte
	// required in the end, therefore using > instead of >= in
	// comparison.
	if (reader != NULL) {
		// Skipping this part if not using binary reader
		// (command which has only ASCII implementation).
		buflen_t wrote_modbus = reader(serial_out, SERIAL_TX_END - serial_out);
		if (serial_out + wrote_modbus > SERIAL_TX_END) goto serial_full;
	}

	// Populate output buffer with final content. Overwriting the
	// content already in the buffer.
	buflen_t wrote = printer(serial_out, SERIAL_TX_END - serial_out);

	// Ensure the content is fitted. Ensure space for NUL or space byte.
	serial_out += wrote;
	if (serial_out+1 >= SERIAL_TX_END) goto serial_full;

	if (buf == NULL) {
		// It's the last, we succeeded. NUL terminating the
		// result string.
		*serial_out = '\0';
		return true;
	} else {
		// Otherwise doing a tail recursion until we run out
		// of input data.
		*serial_out++ = ' ';
		return process_read(buf, serial_out);
	}
	
 serial_full:
	snprintf_P(serial_tx, SERIAL_TX_LEN, PSTR("Serial buffer too short for \"%s\""), name);
	return false;
}

// Process write requests. Outputs error messages or "OK".
static bool process_write(char *buf)
{
	// Finding the name from the lookup table
	char const *const name = strsep(&buf, "=");
	cmd_ascii_t const *const cmd = find_cmd(name);

	if (cmd == NULL) {
		snprintf_P(serial_tx, SERIAL_TX_LEN, PSTR("Unknown field: \"%s\""), name);
		return false;
	}

	// Collecting scanner and writer from PROGMEM storage
	cmd_scan_t const *scanner = pgm_read_ptr_near(&(cmd->scanner));
	cmd_write_t const *writer = pgm_read_ptr_near(&(cmd->action.write));

	if (scanner == NULL) {
		char const *msg = writer == NULL
			? PSTR("Field \"%s\" is not writable")
			: PSTR("Field \"%s\" is writable via Modbus only!");

		snprintf_P(serial_tx, SERIAL_TX_LEN, msg, name);
		return false;
	}

	// We need the value as well
	char *const value = strsep(&buf, " ");
	if (value == NULL) {
		snprintf_P(serial_tx, SERIAL_TX_LEN, PSTR("Field \"%s\" value not set"), name);
		return false;
	}
	
	// OK, now it gets exciting. We have all the functions, so
	// just doing the magic!
	cmd_parse_error = PSTR("Unknown error");
	bool ok = scanner(value, writer);
	if (!ok) {
		int pos = snprintf_P(serial_tx, SERIAL_TX_LEN, PSTR("Field \"%s\" write failed: "), name);
		snprintf_P(serial_tx+pos, SERIAL_TX_LEN-pos, cmd_parse_error, cmd_parse_error_arg);
		return false;
	}

	// If it's the last, we succeeded. Otherwise doing a tail
	// recursion until we run out of data.
	if (buf == NULL) {
		strcpy_P(serial_tx, PSTR("OK"));
		return true;
	}
	return process_write(buf);
}

// Search given command from the table generated to cmd.c
static cmd_ascii_t const *find_cmd(char const *const name)
{
	if (name == NULL) return NULL;
	// Doing binary search over items.
	return bsearch(name, cmd_ascii, cmd_ascii_len, sizeof(cmd_ascii_t), cmd_comparator);
}

// Comparator for cmd_ascii_t.name stored in the PROGMEM.
static int cmd_comparator(const void *key_void, const void *item_void)
{
	const char *key = key_void;
	const cmd_ascii_t *item = item_void;

	// The item is in PROGMEM and the pointer points to PROGMEM, too!
	char const *item_name = pgm_read_ptr_near(&(item->name));
	return strcasecmp_P(key, item_name);
}

// Entry point to this object. Processes given input in ASCII and
// performs the operations in there.
bool interface_ascii(char *buf, buflen_t len)
{
	// Handle corner cases: incorrect line ending or empty
	// message. NB! strip_line_ending alters the buffer!
	if (!strip_line_ending(buf, len)) {
		strcpy_P(serial_tx, PSTR("Message not terminated by newline"));
		return false;
	}

	// Operation type parsing
	char *const op = strsep(&buf, " ");
	if (op != NULL) {
		if (strcasecmp_P(op, PSTR("get")) == 0) {
			if (buf == NULL) {
				strcpy_P(serial_tx, PSTR("   ^ Expecting arguments"));
				return false;
			}
			return process_read(buf, serial_tx);
		}
		if (strcasecmp_P(op, PSTR("set")) == 0) {
			if (buf == NULL) {
				strcpy_P(serial_tx, PSTR("   ^ Expecting arguments"));
				return false;
			}
			return process_write(buf);
		}
	}

	strcpy_P(serial_tx, PSTR("^ Expecting 'GET' or 'SET'"));
	return false;
}
