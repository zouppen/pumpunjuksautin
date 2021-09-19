#pragma once

// Miscellaneous functions for use with command interface.

#include "serial.h"
#include "modbus_types.h"

// Outputs version number.
buflen_t misc_version(char *const buf_out, buflen_t count);

// Formats current time in ISO 8601 format with time zone.
buflen_t misc_now(char *const buf_out, buflen_t count);

// Reads indicator LED status
bool misc_get_led(void);

// Changes indicator LED status
modbus_status_t misc_set_led(bool state);

// Just writes PONG. For testing,
buflen_t misc_pong(char *const buf_out, buflen_t count);
