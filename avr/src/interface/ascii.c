// Pumpunjuksautin ASCII interface functions.
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
#include "ascii.h"
#include "cmd.h"

#define SERIAL_TX_END (serial_tx + SERIAL_TX_LEN)

static bool strip_line_ending(char *const buf, int const len);
static cmd_result_t process_read(char const *name, char **out);
static cmd_result_t process_write(char const *name, char *value);
static bool process_help(void);
static cmd_ascii_t const *find_cmd(char const *const name);
static int cmd_comparator(const void *key_void, const void *item_void);
static void location_aware_error(char const *const ref, cmd_result_t const *const e);

// Version definition is delivered by version.cmake
extern char const version[] PROGMEM;

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
static cmd_result_t process_read(char const *name, char **out)
{
	cmd_ascii_t const *const cmd = find_cmd(name);

	if (cmd == NULL) {
		FAIL(name, "Unknown command");
	}

	// Collecting scanner and writer from PROGMEM storage
	cmd_print_t const *printer = pgm_read_ptr_near(&(cmd->printer));
	cmd_action_t const *action = pgm_read_ptr_near(&(cmd->action));
	void const *reader = pgm_read_ptr_near(&(action->read));

	if (printer == NULL) {
		FAIL(name, "Not readable");
	}

	// Output variable name. Ensure space for an equals sign.
	*out += strlcpy(*out, name, SERIAL_TX_END - *out);
	if (*out + 1 >= SERIAL_TX_END) {
		FAIL(name, "Serial buffer too short for this");
	}
	*(*out)++ = '=';

	// Populate output buffer with the content. Function is
	// responsible for the type safety of void function pointer.
	buflen_t wrote = printer(*out, SERIAL_TX_END - *out, reader);

	// Ensure the content is fitted. Ensure space for a space byte.
	*out += wrote + 1;
	if (*out >= SERIAL_TX_END) {
		FAIL(name, "Serial buffer too short for this");
	}
	*(*out-1) = ' ';
	return cmd_success;
}

static void location_aware_error(char const *const ref, cmd_result_t const *const e)
{
	// String formatting is such a thing which is never easy to
	// read. Some instructive comments added but it won't be too
	// easy, though.

	buflen_t const e_pos = e->error_p - ref;
	buflen_t msg_pos = e_pos + 2;

	// Do we use caret?
	bool caret = msg_pos < SERIAL_TX_LEN;
	bool reprint = true;

	// Initial strategy: Put the message after the caret
	if (caret) {
		// Produce space padding
		memset(serial_tx, ' ', msg_pos);
		serial_tx[msg_pos-2] = '^';

		// Append the error message. It is 16-bit int to avoid overflows
		int msg_len = snprintf_P(serial_tx + msg_pos, SERIAL_TX_LEN - msg_pos, e->error_msg, e->msg_arg1);
		// If the message didn't fit, go for a different strategy.
		if (msg_len + msg_pos >= SERIAL_TX_LEN) {
			if (msg_len + 1 > e_pos) {
				// Doesn't even fit before.
				caret = false;
			} else {
				// Message can fit before the
				// caret. Calculating new position and
				// terminating after the caret.
				msg_pos = e_pos - msg_len - 1;
				serial_tx[e_pos+1] = '\0';
			}
		} else {
			// We were able to fit it, no reprinting of
			// the message needed.
			reprint = false;
		}
	}

	if (reprint) {
		if (!caret) {
			// Output location instead of caret
			msg_pos = snprintf_P(serial_tx, SERIAL_TX_LEN, PSTR("At %d: "), e_pos);
		}
		snprintf_P(serial_tx + msg_pos, SERIAL_TX_LEN - msg_pos, e->error_msg, e->msg_arg1);
		if (caret) {
			// Strip NUL char if ther's caret following
			serial_tx[e_pos-1] = ' ';
		}
	}
}

// Process write requests. Doesn't output anything.
static cmd_result_t process_write(char const *name, char *value)
{
	cmd_ascii_t const *const cmd = find_cmd(name);

	if (cmd == NULL) {
		FAIL(name, "Unknown command");
	}

	// Collecting scanner and writer from PROGMEM storage
	cmd_scan_t const *scanner = pgm_read_ptr_near(&(cmd->scanner));
	cmd_action_t const *action = pgm_read_ptr_near(&(cmd->action));
	void const *writer = pgm_read_ptr_near(&(action->write));

	if (scanner == NULL) {
		FAIL(name, "Not writable");
	}

	// OK, now it gets exciting. We have all the functions, so
	// just doing the magic!
	return scanner(value, writer);
}

static bool process_help()
{
	snprintf_P(serial_tx, SERIAL_TX_LEN, PSTR("Pumpunjuksautin (%S) registers:\n"), version);
	serial_tx_line();
	while (serial_is_transmitting());
	for (int i=0; i<cmd_ascii_len; i++) {
		char const *item_name = pgm_read_ptr_near(&cmd_ascii[i].name);
		bool has_r = pgm_read_ptr_near(&cmd_ascii[i].printer) != NULL;
		bool has_w = pgm_read_ptr_near(&cmd_ascii[i].scanner) != NULL;
		char r = has_r ? 'R': ' ';
		char w = has_w ? 'W': ' ';
		snprintf_P(serial_tx, SERIAL_TX_LEN, PSTR("  %c%c  %S"), r, w, item_name);
		serial_tx_line();
		while (serial_is_transmitting());
	}
	strncpy_P(serial_tx, PSTR("\nUsage:\n\n  [REGISTER[=VALUE]..]\n"), SERIAL_TX_LEN);
	return true;
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
bool ascii_interface(char *buf, buflen_t len)
{
	char const *const buf_start = buf;
	char *out = serial_tx;

	// Handle corner cases: incorrect line ending or empty
	// message. NB! strip_line_ending alters the buffer!
	if (!strip_line_ending(buf, len)) {
		strcpy_P(serial_tx, PSTR("Message not terminated by newline"));
		return false;
	}

	if (buf[0] == '\0') {
		strcpy_P(serial_tx, PSTR("^ Expecting command. Ask for 'HELP'"));
		return false;
	}

	if (strcasecmp_P(buf, PSTR("help")) == 0) {
		return process_help();
	}

	do {
		// Operation type parsing
		char *value = strsep(&buf, " ");
		char *name = strsep(&value, "=");

		cmd_result_t res;
		if (value == NULL) {
			res = process_read(name, &out);
		} else {
			res = process_write(name, value);
		}

		if (res.error_msg != NULL) {
			// We have to craft an error message
			location_aware_error(buf_start, &res);
			return false;
		}
	} while (buf != NULL);

	if (out == serial_tx) {
		// Filling OK if nothing else was written.
		strcpy_P(serial_tx, PSTR("OK"));
	} else {
		// Replace last space with NUL in case something was
		// written.
		*(out-1) = '\0';
	}

	return true;
}
