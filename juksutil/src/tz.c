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

#define _GNU_SOURCE // for asprintf
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <endian.h>
#include "mmap.h"
#include "tz.h"

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(mmap_info_t, mmap_close);

bool mmap_zonefile(char const *zone, mmap_info_t *file);
char *find_version_2(char *p);

bool tz_populate_tzinfo(char const *zone, tzinfo_t *info)
{
	g_auto(mmap_info_t) file;
	if (!mmap_zonefile(zone, &file)) {
		return false;
	}

	char *p = find_version_2(file.data);
	if (p == NULL) {
		printf("ei otsikkoa\n");
	} else {
		printf("on kivaa. eka tavu on %c\n", p[0]);
	}
	return true;
}

bool mmap_zonefile(char const *zone, mmap_info_t *info)
{
	char *filename = NULL;

	if (asprintf(&filename, "/usr/share/zoneinfo/%s", zone == NULL ? "localtime" : zone) == -1) {
		return false;
	}

	bool ok = mmap_open(filename, MMAP_MODE_READONLY, 0, info);
	free(filename);
	return ok;
}

char *find_version_2(char *p)
{
	// Test version
	if (p[4] < '2') return NULL;

	// Find boilerplate
	uint32_t *lens = (uint32_t*)(p + 0x14);

	// Using the obscure variable names from tzfile(5)
	uint32_t tzh_ttisgmtcnt = be32toh(*lens++);
	uint32_t tzh_ttisstdcnt = be32toh(*lens++);
	uint32_t tzh_leapcnt    = be32toh(*lens++);
	uint32_t tzh_timecnt    = be32toh(*lens++);
	uint32_t tzh_typecnt    = be32toh(*lens++);
	uint32_t tzh_charcnt    = be32toh(*lens++);

	int skip = 0x2c; // end of the header
	skip += 4 * tzh_timecnt;
	skip += 1 * tzh_timecnt;
	skip += 6 * tzh_typecnt;
	skip += 4 * tzh_leapcnt;
	skip += 1 * tzh_ttisstdcnt;
	skip += 1 * tzh_ttisgmtcnt;
	skip += tzh_charcnt;

	// Test version
	if (p[skip + 4] < '2') return NULL;
	
	return p + skip;
}

