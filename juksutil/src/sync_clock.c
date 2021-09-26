// Clock synchronization of JuksOS device
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
#include <stdint.h>
#include <inttypes.h>
#include <modbus.h>
#include <time.h>
#include <err.h>
#include "tz.h"

static uint32_t exact_timestamp();

// TODO return errors instead of dying
void sync_clock(tzinfo_t const *tz, bool real_time, char const *dev, int baud, int slave)
{
	// Prepare modbus
	modbus_t *ctx;

	ctx = modbus_new_rtu(dev, baud, 'N', 8, 1);
	if (ctx == NULL) {
		err(1, "Unable to create the libmodbus context");
	}

	if (modbus_connect(ctx)) {
		err(5, "Unable to open serial port");
	}

	if (modbus_set_slave(ctx, slave)) {
		err(5, "Unable to set modbus server id");
	}

	// Collect data and swap endianness
	uint16_t buf[8];
	buf[2] = tz->gmtoff_now >> 16;
	buf[3] = tz->gmtoff_now;
	buf[4] = tz->transition >> 16;
	buf[5] = tz->transition;
	buf[6] = tz->gmtoff_after >> 16;
	buf[7] = tz->gmtoff_after;

	// Sleep until next second and swap endianness of the timestamp
	uint32_t now = real_time ? exact_timestamp() : tz->ref_time;
	buf[0] = now >> 16;
	buf[1] = now;

	if (modbus_write_registers(ctx, 0, 8, buf) != 8) {
		err(2, "Modbus write failed");
	}

	modbus_close(ctx);
	modbus_free(ctx);
}

// Does exact timestamp by sleeping until the next full second
static uint32_t exact_timestamp()
{
	struct timespec tp;
	if (clock_gettime(CLOCK_REALTIME, &tp)) err(1, "Time retrieval failed");

	// Collect the future time
	time_t future = tp.tv_sec + 1;

	// Sleep to the future!
	tp.tv_sec = 0;
	tp.tv_nsec = 1000000000 - tp.tv_nsec;
	if (nanosleep(&tp, NULL)) err(1, "Insomnia!");
	return future;
}

