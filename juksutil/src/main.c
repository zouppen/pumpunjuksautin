// Main module for JuksUtil
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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <glib.h>
#include <time.h>
#include "tz.h"

static gchar *time_zone = "localtime";
static gchar *now_str = NULL;

static GOptionEntry entries[] =
{
	{ "timezone", 'z', 0, G_OPTION_ARG_STRING, &time_zone, "Time zone used in date operations, e.g. Europe/Berlin. Default: localtime", "TZ"},
	{ "now", 'd', 0, G_OPTION_ARG_STRING, &now_str, "Use this time instead of current time. In ISO8601 format", "STRING"},
	{ NULL }
};

int main(int argc, char **argv)
{
	time_t now;
	g_autoptr(GError) error = NULL;
	g_autoptr(GOptionContext) context = g_option_context_new("- Communicates with Pumpunjuksautin");
	g_option_context_add_main_entries(context, entries, NULL);
	if (!g_option_context_parse(context, &argc, &argv, &error))
	{
		errx(1, "Option parsing failed: %s", error->message);
	}

	// Fetching current time
	if (now_str != NULL) {
		// Trying to parse it to UNIX timestamp
		struct tm tm;
		char *end = strptime(now_str, "%FT%T", &tm);
		if (end == NULL || *end != '\0') {
			errx(1, "Invalid date format");
		}

		// Make it do dst lookup when converting the time
		tm.tm_isdst = -1;

		now = mktime(&tm);
	} else {
		now = time(NULL);
	}
	if (now == -1) {
		errx(1, "Fatal date error");
	}
	printf("jes unix sanoo %ld\n", now);

	tzinfo_t info;

	if (!tz_populate_tzinfo(&info, time_zone, now)) {
		err(1, "tzinfo reading failed");
	}

	// Temporary print of values
	printf("%ld %d %ld\n", info.gmtoff_now, info.transition, info.gmtoff_after);
}
