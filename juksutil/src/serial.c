// Serial port configuration
// SPDX-License-Identifier:   GPL-3.0-or-later
// Copyright (C) 2014, 2021 Joel Lehtonen
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

/* Set serial parameters using non-standard baud rate hack:
 * http://stackoverflow.com/questions/3192478/specifying-non-standard-baud-rate-for-ftdi-virtual-serial-port-under-linux
 * and
 * http://stackoverflow.com/questions/4968529/how-to-set-baud-rate-to-307200-on-linux
 * Greetings to Kryptoradio project.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <errno.h>
#include <unistd.h>
#include "serial.h"

static tcflag_t to_speed(const int speed);

FILE *serial_fopen(char *path, int speed)
{
	// Open file descriptor
	int fd = open(path, O_RDWR);
	if (fd == -1) return NULL;

	// Set to raw mode and configure baud rate
	if (!serial_raw(fd, speed)) return NULL;

	// Making it a FILE
	return fdopen(fd, "rb+");
}

bool serial_raw(int fd, int speed)
{
	// Check if speed preset is found or do we need to set a custom speed
	tcflag_t speed_preset = to_speed(speed);
	if (speed_preset == B0) {
		// Find current configuration
		struct serial_struct serial;
		if(ioctl(fd, TIOCGSERIAL, &serial) == -1) return false;
		serial.flags = (serial.flags & ~ASYNC_SPD_MASK) | ASYNC_SPD_CUST;
		serial.custom_divisor = (serial.baud_base + (speed / 2)) / speed;

		// Check that the serial timing error is no more than 2%
		int real_speed = serial.baud_base / serial.custom_divisor;
		if (real_speed < speed * 98 / 100 ||
		    real_speed > speed * 102 / 100) {
			errno = ENOTSUP;
			return false;
		}

		// Activate
		if(ioctl(fd, TIOCSSERIAL, &serial) == -1) return false;

		// Custom baudrate only works with this magic value
		speed_preset = B38400;
	}

	// Start with raw values
	struct termios term;
	term.c_cflag = speed_preset | CS8 | CLOCAL | CREAD;
	cfmakeraw(&term);
	if (tcsetattr(fd,TCSANOW,&term) == -1) return false;

	return true;
}

/**
 * Converts integer to baud rate. If no suitable preset is found,
 * return B0.
 */
static tcflag_t to_speed(const int speed)
{
	switch (speed) {
	case 50: return B50;
	case 75: return B75;
	case 110: return B110;
	case 134: return B134;
	case 150: return B150;
	case 200: return B200;
	case 300: return B300;
	case 600: return B600;
	case 1200: return B1200;
	case 1800: return B1800;
	case 2400: return B2400;
	case 4800: return B4800;
	case 9600: return B9600;
	case 19200: return B19200;
	case 38400: return B38400;
	case 57600: return B57600;
	case 115200: return B115200;
	case 230400: return B230400;
	default: return B0; // Marks "non-standard"
	}
}
