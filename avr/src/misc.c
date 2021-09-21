// Miscellaneous functions for use with command interface.
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
#include <avr/pgmspace.h>
#include "misc.h"
#include "pin.h"
#include "hardware_config.h"

// Version definition is delivered by version.cmake
extern char const version[] PROGMEM;

// Holds the value for a dummy coil
static bool dummy_coil = false;

buflen_t misc_version(char *const buf_out, buflen_t count)
{
	return strlcpy_P(buf_out, version, count);
}

buflen_t misc_now(char *const buf_out, buflen_t count)
{
	time_t now = time(NULL);
	size_t wrote = strftime(buf_out, count, "%FT%T%z", localtime(&now));
	return wrote == 25 ? wrote-1 : BUFLEN_MAX;
}

bool misc_get_led()
{
	return STATE(PIN_LED);
}

modbus_status_t misc_set_led(bool state)
{
	if (state) {
		HIGH(PIN_LED);
	} else {
		LOW(PIN_LED);
	}
	return MODBUS_OK;
}

buflen_t misc_pong(char *const buf_out, buflen_t count)
{
	if (count < 4) return BUFLEN_MAX;

	buf_out[0] = 'P';
	buf_out[1] = 'O';
	buf_out[2] = 'N';
	buf_out[3] = 'G';
	return 4;
}

bool misc_get_dummy_coil()
{
	return dummy_coil;
}

modbus_status_t misc_set_dummy_coil(bool state)
{
	dummy_coil = state;
	return MODBUS_OK;
}
