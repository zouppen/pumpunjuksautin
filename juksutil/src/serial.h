#pragma once

#include <stdio.h>
#include <stdbool.h>

/**
 * Opens a given serial port, configures it as in serial_raw() and
 * returns a FILE handle. In case of an error, NULL is returned and
 * errno is set.
 */
FILE *serial_fopen(char *path, int speed);

/**
 * Configures serial port to use raw 8-bit mode and given baud
 * rate. This function also supports non-standard baud rates if the
 * underlying hardware is capable. If it fails, false is returned and
 * errno is set. Otherwise, true is returned.
 */
bool serial_raw(int fd, int speed);
