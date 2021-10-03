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
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <modbus.h>
#include <time.h>
#include <err.h>
#include <errno.h>
#include <glib.h>
#include "tz.h"

static time_t exact_timestamp();

void sync_clock_get_time_modbus(GString *s, modbus_t *ctx)
{
	uint16_t in[4];
	if (modbus_read_registers(ctx, 0, 4, in) != 4) {
		errx(2, "Modbus read failed: %s", modbus_strerror(errno));
	}
	time_t ts = MODBUS_GET_INT32_FROM_INT16(in, 0);
	int32_t gmtoff = MODBUS_GET_INT32_FROM_INT16(in, 2);
	ts += gmtoff;

	struct tm tm;
	gmtime_r(&ts, &tm);
	tm.tm_gmtoff = gmtoff; // glibc only way to set timezone!
	
	g_string_set_size(s, 30);
	s->len = strftime(s->str, 30, "now=%FT%T%z", &tm);
}

// TODO return errors instead of dying
void sync_clock_modbus(tzinfo_t const *tz, bool real_time, modbus_t *ctx)
{
	// Collect data and swap endianness
	uint16_t buf[8];
	MODBUS_SET_INT32_TO_INT16(buf, 2, tz->gmtoff_now);
	MODBUS_SET_INT32_TO_INT16(buf, 4, tz->transition);
	MODBUS_SET_INT32_TO_INT16(buf, 6, tz->gmtoff_after);

	// Sleep until next second and swap endianness of the timestamp
	uint32_t now = real_time ? exact_timestamp() : tz->ref_time;
	MODBUS_SET_INT32_TO_INT16(buf, 0, now);

	if (modbus_write_registers(ctx, 0, 8, buf) != 8) {
		errx(2, "Modbus write failed: %s", modbus_strerror(errno));
	}
}

void sync_clock_ascii(tzinfo_t const *tz, bool real_time, FILE *f)
{
	// Sleep until next second if real time used
	uint64_t now = real_time ? exact_timestamp() : tz->ref_time;

	// Shouldn't take more than a second
	alarm(1);

	g_autoptr(GString) line_out = g_string_new(NULL);
	g_string_printf(line_out,
			"time=%" PRIu64 " gmtoff=%" PRId32
			" next_turn=%" PRIu64 " gmtoff_turn=%" PRId32 "\n",
			now, tz->gmtoff_now, tz->transition, tz->gmtoff_after);
	fputs(line_out->str, f);
	fflush(f);

	char *line_in = NULL;
	size_t len = 0;
	if (getline(&line_in, &len, f) == -1) {
		errx(1, "Unable to read from serial port");
	}

	// Cancel timeout
	alarm(0);

	if (strcmp("OK\n", line_in)) {
		fputs(line_out->str, stdout);
		fputs(line_in, stdout);
		exit(1);
	}
	free(line_in);
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

