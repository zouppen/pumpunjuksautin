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
#include <unistd.h>
#include <modbus.h>
#include <err.h>
#include <errno.h>
#include <glib.h>
#include <time.h>
#include "tz.h"
#include "sync_clock.h"
#include "serial.h"

static time_t get_timestamp(void);
static tzinfo_t get_tzinfo(void);
static char *format_iso8601(time_t ref);
static char *format_localtime(time_t ref, int32_t gmtoff);
static void cmd_show_transition(void);
static void cmd_sync_clock_modbus();
static void cmd_sync_clock_ascii();
static void cmd_get_time_modbus();
static void cmd_get_time_ascii();
static void cmd_ascii(int const argc, char **argv);
static bool matches(char const *const arg, char const *command, bool const cond);
static void serial_timeout(int signo);
static modbus_t *main_modbus_init(void);
static void main_modbus_free(modbus_t *ctx);
static FILE *main_serial_init(void);

static gchar *time_zone = "localtime";
static gchar *now_str = NULL;
static gchar *dev_path = NULL;
static gint dev_baud = 9600;
static gint dev_slave = 0;

static GOptionEntry entries[] =
{
	{ "timezone", 'z', 0, G_OPTION_ARG_STRING, &time_zone, "Time zone used in date operations, e.g. Europe/Berlin. Default: localtime", "TZ"},
	{ "now", 'n', 0, G_OPTION_ARG_STRING, &now_str, "Use this time instead of current time. In ISO8601 format. Default: now", "STRING"},
	{ "device", 'd', 0, G_OPTION_ARG_FILENAME, &dev_path, "Device path to JuksOS device", "PATH"},
	{ "baud", 'b', 0, G_OPTION_ARG_INT, &dev_baud, "Device baud rate. Default: 9600", "BAUD"},
	{ "slave", 's', 0, G_OPTION_ARG_INT, &dev_slave, "Device Modbus server id. If not defined, ASCII protocol is used", "ID"},
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
					 "  get-time              Get current time from the device.\n"
					 "  sync-time             Synchronize clock of JuksOS device. Sets also DST transition table.\n"
					 "  send KEY[=VALUE]..    Read and/or write values from/to the hardware via ASCII interface\n"
					 "");

	if (!g_option_context_parse(context, &argc, &argv, &error))
	{
		errx(1, "Option parsing failed: %s", error->message);
	}

	// Command validation and selection
	if (argc < 2) {
		errx(1, "Command missing. See %s --help", argv[0]);
	}

	// Set up signal handler for timeouts
	struct sigaction act;
	act.sa_handler = serial_timeout;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (sigaction(SIGALRM, &act, NULL) == -1) {
		err(1, "Unable to set a signal handler");
	}

	// Command parser
	if (matches(argv[1], "show-transition", argc == 2)) {
		// Validate args
		cmd_show_transition();
	} else if (matches(argv[1], "sync-time", argc == 2)) {
		if (dev_slave) {
			cmd_sync_clock_modbus();
		} else {
			cmd_sync_clock_ascii();
		}
	} else if (matches(argv[1], "get-time", argc == 2)) {
		if (dev_slave) {
			cmd_get_time_modbus();
		} else {
			cmd_get_time_ascii();
		}
	} else if (matches(argv[1], "send", argc > 2)) {
		cmd_ascii(argc-2, argv+2);
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

static void cmd_ascii(int const cmds, char **cmd)
{
	if (dev_slave) {
		errx(1, "ASCII interface not available using Modbus. Don't use -s.");
	}
	
	// Craft compound message
	g_autoptr(GString) line_in = g_string_new(cmd[0]);
	for (int i=1; i<cmds; i++) {
		g_string_append_c(line_in, ' ');
		g_string_append(line_in, cmd[i]);
	}
	g_string_append_c(line_in, '\n');

	// Do the serial operations with a timeout
	alarm(1);

	FILE *f = main_serial_init();

	if (fputs(line_in->str, f) == EOF) {
		errx(1, "Unable to write to serial port");
	}

	char *line_out = NULL;
	size_t len = 0;
	if (getline(&line_out, &len, f) == -1) {
		errx(1, "Unable to read from serial port");
	}

	// Cancel timeout
	alarm(0);

	if (line_out[0] == ' ') {
		// Put the command as a reference for the error
		// message.
		fputs(line_in->str, stdout);
	}
	fputs(line_out, stdout);
	free(line_out);
	fclose(f);
}

static void serial_timeout(int signo) {
	errx(1, "ASCII serial protocol timeout. Is the device on and is the baud rate correct?");
}

// Command for getting current time via ASCII
static void cmd_get_time_ascii()
{
	char *s = "now";
	cmd_ascii(1, &s);
}

// Command for getting current time via Modbus
static void cmd_get_time_modbus()
{
	modbus_t *ctx = main_modbus_init();
	
	g_autoptr(GString) s = g_string_new(NULL);
	sync_clock_get_time_modbus(s, ctx);
	puts(s->str);

	main_modbus_free(ctx);
}

// Command for syncing the clock time of a device.
static void cmd_sync_clock_modbus()
{
	modbus_t *ctx = main_modbus_init();
	
	tzinfo_t info = get_tzinfo();
	sync_clock_modbus(&info, now_str == NULL, ctx);

	main_modbus_free(ctx);
}

// Command for syncing the clock time of a device.
static void cmd_sync_clock_ascii()
{
	FILE *f = main_serial_init();

	tzinfo_t info = get_tzinfo();
	sync_clock_ascii(&info, now_str == NULL, f);

	fclose(f);
}

// Command for just showing the next DST transition and the UTC offset
// before and after the transition.
static void cmd_show_transition()
{
	tzinfo_t info = get_tzinfo();
	char *reftime_label = now_str == NULL ?
		"Current time:  ":
		"Reference time:";
	g_autoptr(GString) name = tz_name(time_zone);
	printf("Current and following time zone for \x1b[1m%s\x1b[0m:\n\n", name->str);
	printf("\x1b[1m                  UTC time              Local time                UNIX time   UTC off\x1b[0m\n");
	printf("\x1b[1m%s\x1b[0m   %-22s%-25s%11ld%+9d\n", reftime_label, format_iso8601(info.ref_time), format_localtime(info.ref_time, info.gmtoff_now), info.ref_time, info.gmtoff_now);
	if (info.transition) {
		printf("\x1b[1mNext transition:\x1b[0m  %-22s%-25s%11ld%+9d\n", format_iso8601(info.transition), format_localtime(info.transition, info.gmtoff_after), info.transition, info.gmtoff_after);
	} else {
		printf("\x1b[1mNext transition:\x1b[0m  No future transitions\n");
	}
}

// Returns time formatted to ISO 8601 UTC timestamp. This function is
// not re-entrant. May terminate the program in case of an error.
static char *format_iso8601(time_t ref)
{
	static char buf[25];
	struct tm *tm = gmtime(&ref);
	if (tm == NULL) goto fail;
	if (strftime(buf, sizeof(buf), "%FT%TZ", tm) == 0) goto fail;
	return buf;
 fail:
	errx(1, "Date formatting error");
}

// Returns time formatted to ISO 8601 local time. This function is
// not re-entrant. May terminate the program in case of an error.
static char *format_localtime(time_t ref, int32_t gmtoff)
{
	static char buf[30];
	ref += gmtoff;
	struct tm tm;
	gmtime_r(&ref, &tm);
	tm.tm_gmtoff = gmtoff; // glibc only way to set timezone!
	if (strftime(buf, sizeof(buf), "%FT%T%z", &tm) == 0) goto fail;
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
		goto modbus_error;
	}

	if (modbus_connect(ctx)) {
		goto modbus_error;
	}

	if (modbus_set_slave(ctx, dev_slave)) {
		goto modbus_error;
	}
	
	return ctx;
 modbus_error:
	errx(5, "Modbus communication error: %s", strerror(errno));
}

static void main_modbus_free(modbus_t *ctx)
{
	modbus_close(ctx);
	modbus_free(ctx);
}

static FILE *main_serial_init(void)
{
	if (dev_path == NULL) {
		errx(1, "Device name must be given with -d, optionally baud rate with -b");
	}

	// Serial comms
	FILE *f = serial_fopen(dev_path, dev_baud);
	if (f == NULL) {
		err(1, "Unable to open serial port");
	}
	return f;
}
