// Pumpunjuksautin Modbus interface.
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
#include <avr/pgmspace.h>
#include <stddef.h>
#include <util/crc16.h>
#include "serial.h"
#include "cmd.h"
#include "byteswap.h"

typedef buflen_t function_handler_t(char const *buf, buflen_t len);

typedef struct {
	uint8_t code;
	function_handler_t *f;
} handler_t;

static cmd_modbus_t const *find_cmd(modbus_object_t type, uint16_t addr);
static int cmd_comparator(const void *key_void, const void *item_void);
static function_handler_t *find_function_handler(uint8_t const code);
static int handler_comparator(const void *key_void, const void *item_void);
static buflen_t fill_exception(uint8_t function_code, modbus_status_t status);
static uint16_t modbus_crc(char const *buf, buflen_t len);
static function_handler_t read_input_registers;

// Keep this list numerically sorted
static handler_t const handlers[] PROGMEM = {
	{ 4, &read_input_registers}
	// 1
	// 5
	// 15
	// 6
	// 16
	// 17	
};

// Search given command from the table generated to cmd.c
static cmd_modbus_t const *find_cmd(modbus_object_t type, uint16_t addr)
{
	// Crafting partially initialized key object
	cmd_modbus_t key = {type, addr};

	// Doing binary search over items.
	return bsearch(&key, cmd_modbus, cmd_modbus_len, sizeof(cmd_modbus_t), cmd_comparator);
}

// Comparator for find_cmd
static int cmd_comparator(const void *key_void, const void *item_void)
{
	cmd_modbus_t const *key = key_void;
	cmd_modbus_t const *item_P = item_void;

	// Doing partial comparison up to the actual payload
	return memcmp_P(key, item_P, offsetof(cmd_modbus_t, action));
}

static function_handler_t *find_function_handler(uint8_t const code)
{
	handler_t *h = bsearch(&code, handlers,
			       sizeof(handlers) / sizeof(handler_t),
			       sizeof(handler_t), handler_comparator);
	if (h == NULL) return NULL;
	return pgm_read_ptr_near(&(h->f));
}

static int handler_comparator(const void *key_void, const void *item_void)
{
	uint8_t const *key = key_void;
	handler_t const *item_P = item_void;
	return memcmp_P(key, item_P, offsetof(handler_t, f));
}

static buflen_t read_input_registers(char const *buf, buflen_t len)
{
	// TODO placeholder
	serial_tx[2] = 'k';
	serial_tx[3] = 'i';
	serial_tx[4] = 's';
	serial_tx[5] = 's';
	serial_tx[6] = 'a';
	return 7;
}

uint8_t modbus_get_station_id(void)
{
	// TODO EEPROM storage
	return 1;
}

#if SERIAL_TX_LEN < 5
#error Modbus exceptions require longer serial tx buffer
#endif

static buflen_t fill_exception(uint8_t function_code, modbus_status_t status)
{
	serial_tx[1] = 0x80 + function_code;
	serial_tx[2] = status;
	return 3;
}

// Calculate CRC from given buffer
static uint16_t modbus_crc(char const *buf, buflen_t len)
{
	uint16_t crc = 0xffff;
	for (buflen_t i = 0; i < len; i++) {
		crc = _crc16_update(crc, buf[i]);
	}
	return crc;
}

buflen_t interface_modbus(char *buf, buflen_t len)
{
	// Let's start with RTU structure https://en.wikipedia.org/wiki/Modbus
	
	// Drop clearly invalid data
	if (len < 6) return 0;

	// Check if targeted to us
	if (buf[0] != modbus_get_station_id()) return 0;

	// Collect checksum. NOTE: It is little-endian on wire!
	len -= 2;
	uint16_t const crc_correct = *(uint16_t*)(buf+len);

	// Calculate CRC. If it's incorrect, stop processing the frame
	uint16_t const crc_in = modbus_crc(buf, len);
	if (crc_in != crc_correct) return 0;

	// Start collecting the answer
	serial_tx[0] = buf[0];
	serial_tx[1] = buf[1];
	uint8_t function_code = buf[1];

	// Find handler for function code
	function_handler_t *f = find_function_handler(function_code);
	buflen_t ret;
	if (f == NULL) {
		// Illegal function. Producing reply packet
		ret = fill_exception(function_code, MODBUS_EXCEPTION_ILLEGAL_FUNCTION);
	} else {
		ret = f(buf+1, len-1);
		if (ret+2 > SERIAL_TX_LEN) {
			// Cannot fit CRC
			ret = fill_exception(function_code, MODBUS_EXCEPTION_SLAVE_OR_SERVER_FAILURE);
		}
	}

	// Outbound CRC is written little-endian
	uint16_t crc_out = modbus_crc(serial_tx, ret);
	*(uint16_t*)(serial_tx+ret) = crc_out;
	return ret + 2;
}


//	uint16_t address = bswap_16(*(uint16_t*)(buf+2));
	
	// TODO now we just print how it went
//	snprintf_P(serial_tx, SERIAL_TX_LEN, PSTR("%d / %d"), crc_correct, crc);
//	return true;*/

