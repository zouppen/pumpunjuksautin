// Time zone information digger
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
#include <glib.h>
#include <endian.h>
#include "mmap.h"
#include "tz.h"

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(mmap_info_t, mmap_close);

static bool mmap_zonefile(char const *zone, mmap_info_t *file);
static void *find_v2_header(char *p, int len);
static bool find_next_transition(tzinfo_t *dest, char *p, int len, int64_t now);

static int const header_len = 0x2c;
static int const header_boilerplate = 0x14;

typedef struct __attribute__((packed)) {
	int32_t        tt_gmtoff;
	unsigned char  tt_isdst;
	unsigned char  tt_abbrind;
} ttinfo_t;

bool tz_populate_tzinfo(tzinfo_t *dest, char const *zone, int64_t now)
{
	g_auto(mmap_info_t) file;
	if (!mmap_zonefile(zone, &file)) {
		return false;
	}

	void *p = find_v2_header(file.data, file.length);
	if (p == NULL) {
		return false;
	}

	find_next_transition(dest, p, file.data + file.length - p, now);
	return true;
}

// Open given zone file with mmap()
static bool mmap_zonefile(char const *zone, mmap_info_t *info)
{
	g_autoptr(GString) filename = g_string_new(NULL);
	g_string_printf(filename, "/usr/share/zoneinfo/%s", zone);

	bool ok = mmap_open(filename->str, MMAP_MODE_READONLY, info);
	return ok;
}

// Iterates through tzfile and skips the legacy header. Returns
// pointer to the beginning of version 2 header. Returns NULL if file
// format is invalid.
static void *find_v2_header(char *p, int len)
{
	// Check if it's too short
	if (len < header_len) return NULL;

	// Test version
	if (p[4] < '2') return NULL;

	// Find boilerplate
	uint32_t *lens = (uint32_t*)(p + header_boilerplate);

	// Using the obscure variable names from tzfile(5)
	uint32_t tzh_ttisgmtcnt = be32toh(lens[0]);
	uint32_t tzh_ttisstdcnt = be32toh(lens[1]);
	uint32_t tzh_leapcnt    = be32toh(lens[2]);
	uint32_t tzh_timecnt    = be32toh(lens[3]);
	uint32_t tzh_typecnt    = be32toh(lens[4]);
	uint32_t tzh_charcnt    = be32toh(lens[5]);

	int const skip = header_len +
		4 * tzh_timecnt +
		1 * tzh_timecnt +
		6 * tzh_typecnt +
		4 * tzh_leapcnt +
		1 * tzh_ttisstdcnt +
		1 * tzh_ttisgmtcnt +
		tzh_charcnt;

	// Check that we're still inside the file
	if (len < skip) return NULL;

	return p + skip;
}

// Find the next transition from version 2 part of the tzfile.
static bool find_next_transition(tzinfo_t *dest, char *p, int len, int64_t now)
{
	// Check if it's too short
	if (len < header_len) return false;

	// Test version
	if (p[4] < '2') return false;

	// Find boilerplate
	uint32_t *lens = (uint32_t*)(p + header_boilerplate);

	// Getting only the parts we are interested in.
	uint32_t tzh_timecnt    = be32toh(lens[3]);
	uint32_t tzh_typecnt    = be32toh(lens[4]);

	int64_t *transition = (void *)(p + header_len);
	uint8_t *type = (void *)(transition + tzh_timecnt);
	ttinfo_t *info = (void *)(type + tzh_timecnt);

	if ((void *)p + len < (void *)(info + tzh_typecnt)) {
		// EOF reached
		return false;
	}

	// Binary search
	uint32_t low = 0, high = tzh_timecnt;
	while (low+1 < high) {
		uint32_t mid = (high+low)/2;
		int64_t ts_mid = be64toh(transition[mid]);
		if (now < ts_mid) {
			high = mid;
		} else {
			low = mid;
		}
	}

	// Find gmtoff
	ttinfo_t *ttinfo_low = &info[type[low]];
	ttinfo_t *ttinfo_high = (high == tzh_timecnt) ? NULL : &info[type[high]];

	// Populate answer
	dest->gmtoff_now = be32toh(ttinfo_low->tt_gmtoff);
	if (ttinfo_high == NULL) {
		dest->transition = 0;
		dest->gmtoff_after = 0;
	} else {
		dest->transition = be64toh(transition[high]);
		dest->gmtoff_after = be32toh(ttinfo_high->tt_gmtoff);
	}

	dest->ref_time = now;
	return true;
}
