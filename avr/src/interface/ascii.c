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

static bool process_read(char *buf, char *serial_out, buflen_t parse_pos);
static bool process_write(char *buf, buflen_t parse_pos);
static bool process_help(void);
static cmd_ascii_t const *find_cmd(char const *const name);
static int cmd_comparator(const void *key_void, const void *item_void);
static buflen_t serial_pad(buflen_t n);
static void error_full(buflen_t parse_pos);

// Version definition is delivered by version.cmake
extern char const version[] PROGMEM;

bool ascii_strip_line_ending(char *const buf, int const len)
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

static buflen_t serial_pad(buflen_t n)
{
	if (n + 2 < SERIAL_TX_LEN) {
		// Produce space padding
		memset(serial_tx, ' ', n+2);
		serial_tx[n] = '^';
		return n+2;
	} else {
		// Otherwise just col position
		return snprintf_P(serial_tx, SERIAL_TX_LEN, PSTR("At %d: "), n);
	}
}

// Process read requests, writing to serial buffer.
static bool process_read(char *buf, char *serial_out, buflen_t parse_pos)
{
	const char *const ref_p = buf - parse_pos;

	// Collecting parameter name to read
	char const *const name = strsep(&buf, " ");
	cmd_ascii_t const *const cmd = find_cmd(name);

	if (cmd == NULL) {
		const buflen_t pad = serial_pad(parse_pos);
		strlcpy_P(serial_tx + pad, PSTR("Unknown field"), SERIAL_TX_LEN - pad);
		return false;
	}

	// Collecting scanner and writer from PROGMEM storage
	cmd_print_t const *printer = pgm_read_ptr_near(&(cmd->printer));
	cmd_action_t const *action = pgm_read_ptr_near(&(cmd->action));
	void const *reader = pgm_read_ptr_near(&(action->read));

	if (printer == NULL) {
		const buflen_t pad = serial_pad(parse_pos);
		strlcpy_P(serial_tx + pad, PSTR("Not readable"), SERIAL_TX_LEN - pad);
		return false;
	}

	// Output variable name. Ensure space for an equals sign.
	serial_out += strlcpy(serial_out, name, SERIAL_TX_END - serial_out);
	if (serial_out+1 >= SERIAL_TX_END) goto serial_full;
	*serial_out++ = '=';

	// Populate output buffer with the content. Function is
	// responsible for the type safety of void function pointer.
	buflen_t wrote = printer(serial_out, SERIAL_TX_END - serial_out, reader);

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
		return process_read(buf, serial_out, buf-ref_p);
	}
	
 serial_full:
	error_full(parse_pos);
	return false;
}

static void error_full(buflen_t parse_pos)
{
	// String formatting is such a thing which is never easy to
	// read. Not even trying to explain this. In case there are
	// bugs found here, just rewrite this function.
	const buflen_t pad = serial_pad(parse_pos);
	int msg_len = 32;
	int msg_pos = pad;
	int tail = 0;
	if (msg_pos + msg_len >= SERIAL_TX_LEN) {
		// Put it before ^ sign
		msg_pos = pad - 3 - msg_len;
		if (msg_pos < 0) {
			// If not enough, just print the message
			msg_pos = 0;
			if (msg_len >= SERIAL_TX_LEN) msg_len = SERIAL_TX_LEN-1;
		} else {
			tail = 2;
		}
	}

	strncpy_P(serial_tx + msg_pos, PSTR("Serial buffer too short for this"), msg_len);
	// Terminate string
	serial_tx[msg_pos + msg_len + tail] = '\0';
}

// Process write requests. Outputs error messages or "OK".
static bool process_write(char *buf, buflen_t parse_pos)
{
	const char *const ref_p = buf;

	// Finding the name from the lookup table
	char const *const name = strsep(&buf, "=");
	cmd_ascii_t const *const cmd = find_cmd(name);

	if (cmd == NULL) {
		const buflen_t pad = serial_pad(parse_pos);
		snprintf_P(serial_tx+pad, SERIAL_TX_LEN-pad, PSTR("Unknown field"));
		return false;
	}

	// Collecting scanner and writer from PROGMEM storage
	cmd_scan_t const *scanner = pgm_read_ptr_near(&(cmd->scanner));
	cmd_action_t const *action = pgm_read_ptr_near(&(cmd->action));
	void const *writer = pgm_read_ptr_near(&(action->write));

	if (scanner == NULL) {
		const buflen_t pad = serial_pad(parse_pos);
		snprintf_P(serial_tx+pad, SERIAL_TX_LEN-pad, PSTR("Not writable"), name);
		return false;
	}

	// We need the value as well
	char *const value = strsep(&buf, " ");
	if (value == NULL) {
		const buflen_t pad = serial_pad(parse_pos+strlen(name));
		snprintf_P(serial_tx + pad, SERIAL_TX_LEN - pad, PSTR("Expected '='"));
		return false;
	}
	
	// OK, now it gets exciting. We have all the functions, so
	// just doing the magic!
	buflen_t const val_pos = value - ref_p + parse_pos;
	const cmd_result_t res = scanner(value, writer);
	if (res.error_msg != NULL) {
		const buflen_t pad = serial_pad(val_pos + res.error_pos);
		snprintf_P(serial_tx+pad, SERIAL_TX_LEN-pad, res.error_msg, res.msg_arg1);
		return false;
	}

	// If it's the last, we succeeded. Otherwise doing a tail
	// recursion until we run out of data.
	if (buf == NULL) {
		return true;
	}
	return process_write(buf, buf - ref_p + parse_pos);
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
	strncpy_P(serial_tx, PSTR("\nUsage:\n\n  GET [REGISTER]...\n  SET [REGISTER=VALUE]...\n"), SERIAL_TX_LEN);
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
bool ascii_interface(char *buf)
{
	const char *const ref_p = buf;

	// Operation type parsing
	char *const op = strsep(&buf, " ");
	if (op != NULL) {
		if (strcasecmp_P(op, PSTR("get")) == 0) {
			if (buf == NULL) {
				strcpy_P(serial_tx, PSTR("   ^ Expecting arguments"));
				return false;
			}
			return process_read(buf, serial_tx, buf-ref_p);
		}
		if (strcasecmp_P(op, PSTR("set")) == 0) {
			if (buf == NULL) {
				strcpy_P(serial_tx, PSTR("   ^ Expecting arguments"));
				return false;
			}
			strcpy_P(serial_tx, PSTR("OK"));
			return process_write(buf, buf-ref_p);
		}
		if (strcasecmp_P(op, PSTR("help")) == 0) {
			if (buf != NULL) {
				strcpy_P(serial_tx, PSTR("    ^ No arguments expected"));
				return false;
			}
			return process_help();
		}

	}

	strcpy_P(serial_tx, PSTR("^ Expecting 'GET', 'SET', or 'HELP'"));
	return false;
}
