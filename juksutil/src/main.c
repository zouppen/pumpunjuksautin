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

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include "tz.h"

int main(int argc, char **argv)
{
	tzinfo_t info;
	if (argc != 3) errx(1, "Parametreja väärä määrä: %d", argc);
	if (!tz_populate_tzinfo(&info, argv[1], atoi(argv[2]))) {
		err(1, "tzinfo reading failed");
	}

	// Temporary print of values
	printf("%ld %d %ld\n", info.gmtoff_now, info.transition, info.gmtoff_after);
}
