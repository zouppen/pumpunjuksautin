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
#include <modbus.h>
#include <err.h>
#include <errno.h>
#include <glib.h>
#include <time.h>
#include "tz.h"
#include "sync_clock.h"

static time_t get_timestamp(void);
static tzinfo_t get_tzinfo(void);
static char *format_iso8601(time_t ref);
static void cmd_show_transition(void);
static void cmd_sync_clock();
static void cmd_ascii(int const argc, char **argv);
static bool matches(char const *const arg, char const *command, bool const cond);
static modbus_t *main_modbus_init(void);
static void main_modbus_free(modbus_t *ctx);

static gchar *time_zone = "localtime";
static gchar *now_str = NULL;
static gchar *dev_path = NULL;
static gint dev_baud = 9600;
static gint dev_slave = 1;

static GOptionEntry entries[] =
{
	{ "timezone", 'z', 0, G_OPTION_ARG_STRING, &time_zone, "Time zone used in date operations, e.g. Europe/Berlin. Default: localtime", "TZ"},
	{ "now", 'n', 0, G_OPTION_ARG_STRING, &now_str, "Use this time instead of current time. In ISO8601 format. Default: now", "STRING"},
	{ "device", 'd', 0, G_OPTION_ARG_FILENAME, &dev_path, "Device path to JuksOS device", "PATH"},
	{ "baud", 'b', 0, G_OPTION_ARG_INT, &dev_baud, "Device baud rate. Default: 9600", "BAUD"},
	{ "slave", 's', 0, G_OPTION_ARG_INT, &dev_slave, "Device Modbus server id. Default: 1", "ID"},
	{ NULL }
};

int main(int argc, char **argv)
{
	g_autoptr(GError) error = NULL;
	g_autoptr(GOptionContext) context = g_option_context_new("COMMAND");
	g_option_context_add_main_entries(context, entries, NULL);
	g_option_context_set_summary(context, "Tool for communicating with JuksOS devices such as Pumpunjuksautin.");
	g_option_context_set_description(context,
					 "Commands:\n"
					 "  show-transition       Show current time zone and future DST transition, if any.\n"
					 "  sync-clock            Synchronize clock of JuksOS device. Sets also DST transition table.\n"
					 "  get KEY..             Read values from the hardware via ASCII interface\n"
					 "  set KEY=VALUE..       Write values to the hardware via ASCII interface"
					 "");

	if (!g_option_context_parse(context, &argc, &argv, &error))
	{
		errx(1, "Option parsing failed: %s", error->message);
	}

	// Command validation and selection
	if (argc < 2) {
		errx(1, "Command missing. See %s --help", argv[0]);
	}
	if (matches(argv[1], "show-transition", argc == 2)) {
		// Validate args
		cmd_show_transition();
	} else if (matches(argv[1], "sync-clock", argc == 2)) {
		cmd_sync_clock();
	} else if (matches(argv[1], "get", argc > 2) ||
		   matches(argv[1], "set", argc > 2)) {
		cmd_ascii(argc, argv);
	} else {
		errx(1, "Invalid command name. See %s --help", argv[0]);
	}
	return 0;
}

static bool matches(char const *const arg, char const *command, bool const cond)
{
	if (strcmp(arg, command)) return false;
	if (!cond) errx(1, "Invalid number of arguments to %s. See --help", command);
	return true;
}

static void cmd_ascii(int const argc, char **argv)
{
	modbus_t *ctx = main_modbus_init();

	// Craft compound message
	g_autoptr(GString) cmd = g_string_new(NULL);
	g_string_append_c(cmd, dev_slave);
	g_string_append_c(cmd, 0x1C); // Custom "ASCII wrapper" command
	g_string_append(cmd, argv[1]);
	for (int i=2; i<argc; i++) {
		g_string_append_c(cmd, ' ');
		g_string_append(cmd, argv[i]);
	}

	// Send it over Modbus, including the terminating NUL char.
	if (modbus_send_raw_request(ctx, (uint8_t*)(cmd->str), cmd->len+1) == -1) {
		errx(1, "Unable to send raw request: %s", modbus_strerror(errno));
	}

	// Retrieving the answer payload
	uint8_t rsp[MODBUS_MAX_ADU_LENGTH];
	int len = modbus_receive_confirmation(ctx, rsp);
	if (len == -1) {
		errx(1, "Communication problem: %s", modbus_strerror(errno));
	}
	if (len < 3) {
		errx(1, "Response was too short (%d bytes)", len);
	}
	if (rsp[1] != dev_slave) {
		errx(1, "Invalid server id (0x%02x)", rsp[0]);
	}
	if (rsp[1] != 0x1C) {
		errx(1, "Response was incorrect (0x%02x)", rsp[0]);
	}

	// Now we are on the safe side, ready to print
	puts((char*)(rsp+2));

	main_modbus_free(ctx);
}

// Command for syncing the clock time of a device.
static void cmd_sync_clock()
{
	modbus_t *ctx = main_modbus_init();
	
	tzinfo_t info = get_tzinfo();
	sync_clock(&info, now_str == NULL, ctx);

	main_modbus_free(ctx);
}

// Command for just showing the next DST transition and the UTC offset
// before and after the transition.
static void cmd_show_transition()
{
	tzinfo_t info = get_tzinfo();
	ldiv_t off_ref = ldiv(info.gmtoff_now / 60, 60);
	printf("\x1b[1m                  UTC time (ISO 8601)  Zone    UNIX time   UTC offset\x1b[0m\n");
	printf("\x1b[1mReference time:\x1b[0m   %-19s  %+03ld:%02ld%12ld  %+10d\n", format_iso8601(info.ref_time), off_ref.quot, off_ref.rem, info.ref_time, info.gmtoff_now);
	if (info.transition) {
		ldiv_t off_after = ldiv(info.gmtoff_after / 60, 60);
		printf("\x1b[1mNext transition:\x1b[0m  %-19s  %+03ld:%02ld%12ld  %+10d\n", format_iso8601(info.transition), off_after.quot, off_after.rem, info.transition, info.gmtoff_after);
	} else {
		printf("\x1b[1mNext transition:\x1b[0m  No future transitions\n");
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

static modbus_t *main_modbus_init(void)
{
	if (dev_path == NULL) {
		errx(1, "Device name must be given with -d, optionally baud rate with -b and server id with -s");
	}

	// Prepare modbus
	modbus_t *ctx = modbus_new_rtu(dev_path, dev_baud, 'N', 8, 1);
	if (ctx == NULL) {
		err(1, "Unable to create the libmodbus context");
	}

	if (modbus_connect(ctx)) {
		err(5, "Unable to open serial port");
	}

	if (modbus_set_slave(ctx, dev_slave)) {
		err(5, "Unable to set modbus server id");
	}
	
	return ctx;
}

static void main_modbus_free(modbus_t *ctx)
{
	modbus_close(ctx);
	modbus_free(ctx);
}
