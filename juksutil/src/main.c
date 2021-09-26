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

static time_t get_timestamp(void);
static tzinfo_t get_tzinfo(void);
static char *format_iso8601(time_t ref);

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
	g_autoptr(GError) error = NULL;
	g_autoptr(GOptionContext) context = g_option_context_new("COMMAND");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "Tool for communicating with JuksOS devices such as Pumpunjuksautin.");
	g_option_context_set_description(context, "Commands:\n  show-transition       Show current time zone and future DST transition, if any.\n");

	if (!g_option_context_parse(context, &argc, &argv, &error))
	{
		errx(1, "Option parsing failed: %s", error->message);
	}

	// Command validation and selection
	if (argc < 2) {
		errx(1, "Command missing. See %s --help", argv[0]);
	}
	if (argc > 2) {
		errx(1, "Too many arguments. See %s --help", argv[0]);
	}
	if (!strcmp(argv[1], "show-transition")) {
		tzinfo_t info = get_tzinfo();
		ldiv_t off_ref = ldiv(info.gmtoff_now / 60, 60);
		printf("\x1b[1m                  ISO 8601 UTC time    Zone    UNIX time   UTC offset\x1b[0m\n");
		printf("\x1b[1mReference time:\x1b[0m   %-19s  %+03ld:%02ld%12ld  %+10d\n", format_iso8601(info.ref_time), off_ref.quot, off_ref.rem, info.ref_time, info.gmtoff_now);
		if (info.transition) {
			ldiv_t off_after = ldiv(info.gmtoff_after / 60, 60);
			printf("\x1b[1mNext transition:\x1b[0m  %-19s  %+03ld:%02ld%12ld  %+10d\n", format_iso8601(info.transition), off_after.quot, off_after.rem, info.transition, info.gmtoff_after);
		} else {
			printf("\x1b[1mNext transition:\x1b[0m  No future transitions\n");
		}
	} else {
		errx(1, "Invalid command name. See %s --help", argv[0]);
	}
}

// Returns time formatted to ISO8601 UTC timestamp. This function is
// not re-entrant. May terminate the program in case of an error.
static char *format_iso8601(time_t ref)
{
	static char buf[25];
	struct tm *tm = gmtime(&ref);
	if (tm == NULL) goto fail;
	if (strftime(buf, sizeof(buf), "%FT%T", tm) == 0) goto fail;
	return buf;
 fail:
	errx(1, "Date formatting error");
}

// Get timestamp from global variable if set or use the current
// time. May terminate the program if invalid time given.
static time_t get_timestamp()
{
	time_t now;

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
	return now;
}

// Get time zone information based on global variables. May terminate
// the program if invalid time or zone given.
tzinfo_t get_tzinfo()
{
	tzinfo_t info;
	if (!tz_populate_tzinfo(&info, time_zone, get_timestamp())) {
		err(1, "tzinfo reading failed");
	}
	return info;
}
