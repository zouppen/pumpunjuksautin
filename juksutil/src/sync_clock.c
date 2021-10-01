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
#include <errno.h>
#include "tz.h"

static time_t exact_timestamp();

// TODO return errors instead of dying
void sync_clock_modbus(tzinfo_t const *tz, bool real_time, modbus_t *ctx)
{
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
		errx(2, "Modbus write failed: %s", modbus_strerror(errno));
	}
}

void sync_clock_ascii(tzinfo_t const *tz, bool real_time, FILE *f)
{
	// Sleep until next second if real time used
	uint64_t now = real_time ? exact_timestamp() : tz->ref_time;

	fprintf(f, "time=%" PRIu64 " gmtoff=%" PRId32
		" next_turn=%" PRIu64 " gmtoff_turn=%" PRId32 "\n",
		now, tz->gmtoff_now, tz->transition, tz->gmtoff_after);
	fflush(f);
}

// Does exact timestamp by sleeping until the next full second
static time_t exact_timestamp()
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

