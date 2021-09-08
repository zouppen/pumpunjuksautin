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

static bool strip_line_ending(char *buf, int const len);
static bool process_read(char *buf);
static bool process_write(char *buf);
static cmd_ascii_t *find_cmd(char const *const name);
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

static bool process_read(char *buf)
{
	strcpy_P(serial_tx, PSTR("Not yet implemented"));
	// Valid command though, so answering true
	return true;
}

static bool process_write(char *buf)
{
	// Finding the name from the lookup table
	char *const name = strsep(&buf, "=");
	cmd_ascii_t *cmd = find_cmd(name);

	if (cmd == NULL) {
		// TODO: See issue #15
		snprintf_P(serial_tx, SERIAL_TX_LEN, PSTR("Unknown field: \"%1s\""), name);
		return false;
	}

	// Collecting scanner and writer from PROGMEM storage
	cmd_scan_t const *scanner = pgm_read_ptr_near(&(cmd->scanner));
	cmd_write_t const *writer = pgm_read_ptr_near(&(cmd->action.write));

	if (writer == NULL) {
		snprintf_P(serial_tx, SERIAL_TX_LEN, PSTR("Field \"%s\" is not writabled"), name);
		return false;
	}	
	if (scanner == NULL) {
		snprintf_P(serial_tx, SERIAL_TX_LEN, PSTR("Field \"%s\" doesn't have scanner"), name);
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
	bool ok = scanner(value, writer);
	if (!ok) {
		snprintf_P(serial_tx, SERIAL_TX_LEN, PSTR("Field \"%s\" write failed"), name);
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
static cmd_ascii_t *find_cmd(char const *const name)
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
	if (strcasecmp_P(op, PSTR("read")) == 0) {
		return process_read(buf);
	}
	if (strcasecmp_P(op, PSTR("write")) == 0) {
		return process_write(buf);
	}

	strcpy_P(serial_tx, PSTR("Unknown operation. Expecting READ or WRITE"));
	return false;
}
