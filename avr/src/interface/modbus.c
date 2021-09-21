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

#include <string.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <stddef.h>
#include <util/crc16.h>
#include "modbus.h"
#include "cmd.h"
#include "../byteswap.h"

typedef buflen_t function_handler_t(char const *buf, buflen_t len);

typedef struct {
	uint8_t code;
	function_handler_t *f;
} handler_t;

static cmd_modbus_t const *find_cmd(modbus_object_t type, uint16_t addr);
static int cmd_comparator(const void *key_void, const void *item_void);
static function_handler_t *find_function_handler(uint8_t const code);
static int handler_comparator(const void *key_void, const void *item_void);
static cmd_modbus_result_t try_register_write(uint16_t const addr, char const *buf_in, buflen_t const len);
static buflen_t fill_exception(modbus_status_t status);
static cmd_modbus_result_t wrap_exception(modbus_status_t status);
static uint16_t modbus_crc(char const *buf, buflen_t len);
static function_handler_t read_registers;
static function_handler_t write_register;
static function_handler_t write_registers;

// Keep this list numerically sorted
static handler_t const handlers[] PROGMEM = {
	{ 0x03, &read_registers},
	{ 0x04, &read_registers},
	{ 0x06, &write_register},
	{ 0x10, &write_registers},
	// 1
	// 5
	// 15
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

static buflen_t read_registers(char const *buf, buflen_t len)
{
	// Command type is already populated in the tx
	// buffer. Determining the correct command type from there.
	modbus_object_t const type = serial_tx[1] == 3 ? HOLDING_REGISTER : INPUT_REGISTER;
	buflen_t const tx_header_len = 3;

	if (len != 4) {
		return fill_exception(MODBUS_EXCEPTION_ILLEGAL_FUNCTION);
	}

	// Parse rest of the packet
	uint16_t base_addr = bswap_16(*(uint16_t*)buf);
	uint16_t registers = bswap_16(*(uint16_t*)(buf+2));
	if (registers > (SERIAL_TX_LEN-tx_header_len) / 2) {
		// Wouldn't fit to the output buffer
		return fill_exception(MODBUS_EXCEPTION_SLAVE_OR_SERVER_FAILURE);
	}
	serial_tx[2] = 2*registers;

	// Start the actual retrieval process
	for (uint16_t i = 0; i < registers; ) {
		// Retrieve suitable handler, if any
		cmd_modbus_t const *cmd = find_cmd(type, base_addr+i);
		if (cmd == NULL) {
			return fill_exception(MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS);
		}

		// Retrieving data from PROGMEM
		cmd_bin_read_t const *reader = pgm_read_ptr_near(&(cmd->reader));
		cmd_action_t const *action = pgm_read_ptr_near(&(cmd->action));
		void const *getter = pgm_read_ptr_near(&(action->read));
		if (reader == NULL) {
			// Not readable
			return fill_exception(MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS);
		}

		// Actual filling of data
		cmd_modbus_result_t r = reader(serial_tx+tx_header_len+2*i, 2*(registers-i), getter);
		if (r.code != MODBUS_OK) {
			// Passing error
			return fill_exception(r.code);
		}
		if ((r.consumed & 1) != 0) {
			// If the amount is not aligned to register
			// width then something is bad.
			return fill_exception(MODBUS_EXCEPTION_SLAVE_OR_SERVER_FAILURE);
		}

		i += r.consumed / 2;
	}

	return tx_header_len + 2*registers;
}

// Write a single holding registers (function code 16)
static buflen_t write_register(char const *buf, buflen_t len)
{
	// Single register write has constant length
	if (len != 4) {
		return fill_exception(MODBUS_EXCEPTION_ILLEGAL_FUNCTION);
	}

	// Response payload is identical to the request
	memcpy(serial_tx+2, buf, 4);

	// Getting address and running write once
	uint16_t addr = bswap_16(*(uint16_t*)buf);
	cmd_modbus_result_t r = try_register_write(addr, buf+2, 2);
	if (r.code != MODBUS_OK) {
		return fill_exception(r.code);
	} else {
		return 6;
	}
}

// Write many holding registers (function code 16)
static buflen_t write_registers(char const *buf, buflen_t len)
{
	// There are 5 bytes minimum after the function code
	buflen_t const input_header_len = 5;

	if (len < input_header_len) {
		return fill_exception(MODBUS_EXCEPTION_ILLEGAL_FUNCTION);
	}

	// Response payload has the same boilerplate but only 4 bytes
	// (byte count is left out)
	memcpy(serial_tx+2, buf, 4);

	// Collect base address and bytes but ignore number of
	// registers (indices 2 and 3). It is redundant information.
	uint16_t const base_addr = bswap_16(*(uint16_t*)buf);
	buflen_t const bytes = buf[4];

	// Make sure all input data is there
	if (input_header_len + bytes != len) {
		return fill_exception(MODBUS_EXCEPTION_ILLEGAL_FUNCTION);
	}
	
	// Start with base address and iterate until everything is got.
	for (buflen_t i=0; i < bytes; ) {
		cmd_modbus_result_t r = try_register_write(base_addr+i/2, buf+input_header_len+i, bytes-i);
		if (r.code != MODBUS_OK) {
			return fill_exception(r.code);
		}
		if ((r.consumed & 1) != 0) {
			// If the amount is not aligned to register
			// width then something is bad.
			return fill_exception(MODBUS_EXCEPTION_SLAVE_OR_SERVER_FAILURE);
		}
		i += r.consumed;
	}
	return 6;
}

static cmd_modbus_result_t try_register_write(uint16_t const addr, char const *buf_in, buflen_t const len)
{
	// Find command responsible of this address
	cmd_modbus_t const *cmd = find_cmd(HOLDING_REGISTER, addr);
	if (cmd == NULL) {
		// Command not found
		return wrap_exception(MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS);
	}

	cmd_bin_write_t const *writer = pgm_read_ptr_near(&(cmd->writer));
	cmd_action_t const *action = pgm_read_ptr_near(&(cmd->action));
	void const *setter = pgm_read_ptr_near(&(action->write));
	if (writer == NULL) {
		// Not readable
		return wrap_exception(MODBUS_EXCEPTION_ILLEGAL_DATA_ADDRESS);
	}

	// Finally we can do the actual action.
	return writer(buf_in, len, setter);
}

uint8_t modbus_get_station_id(void)
{
	// TODO EEPROM storage
	return 1;
}

#if SERIAL_TX_LEN < 8
#error Modbus exceptions and typical answers require longer serial tx buffer
#endif

static buflen_t fill_exception(modbus_status_t status)
{
	serial_tx[1] |= 0x80;
	serial_tx[2] = status;
	return 3;
}

static cmd_modbus_result_t wrap_exception(modbus_status_t status)
{
	// Pass the result and report no data consumed
	cmd_modbus_result_t a = { status, 0};
	return a;
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

buflen_t modbus_interface(char *buf, buflen_t len)
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
		ret = fill_exception(MODBUS_EXCEPTION_ILLEGAL_FUNCTION);
	} else {
		ret = f(buf+2, len-2);
		if (ret+2 > SERIAL_TX_LEN) {
			// Cannot fit CRC
			ret = fill_exception(MODBUS_EXCEPTION_SLAVE_OR_SERVER_FAILURE);
		}
	}

	// Outbound CRC is written little-endian
	uint16_t crc_out = modbus_crc(serial_tx, ret);
	*(uint16_t*)(serial_tx+ret) = crc_out;
	return ret + 2;
}
