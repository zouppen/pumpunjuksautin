// Helper for mmap()
// SPDX-License-Identifier:   GPL-3.0-or-later
// Copyright (C) 2013-2021 Joel Lehtonen
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include "mmap.h"

bool mmap_open(const char *pathname, mmap_mode_t mode, size_t length, mmap_info_t *info)
{
	// Make the g_auto work correctly in case glib is used.
	info->data = NULL;

	int open_flags, mmap_prot, mmap_flags; // Flags depending on mode.

	// Determining options for open(2) and mmap(2).
	switch (mode) {
	case (MMAP_MODE_READONLY):
		open_flags = O_RDONLY;
		mmap_prot = PROT_READ;
		mmap_flags = MAP_PRIVATE;
		break;
	case (MMAP_MODE_WRITE):
		open_flags = O_RDWR;
		mmap_prot = PROT_READ | PROT_WRITE;
		mmap_flags = MAP_SHARED;
		break;
	case (MMAP_MODE_VOLATILE_WRITE):
		open_flags = O_RDONLY;
		mmap_prot = PROT_READ | PROT_WRITE;
		mmap_flags = MAP_PRIVATE;
		break;
	default:
		errno = EINVAL;
		goto fail_start;
	}

	// Opening a file as we do normally.
	info->fd = open(pathname, open_flags);
	if (info->fd == -1) goto fail_start;
	
	if (length == 0) {
		// Determining the length of this file.
		struct stat stats;
		if (fstat(info->fd, &stats) == -1) goto fail_open;
		info->length = stats.st_size;
	} else {
		// Using predefined length
		info->length = length;
	}

	// Doing some black bit magic with mmap.
	info->data = mmap(NULL, info->length, mmap_prot, mmap_flags, info->fd, 0);
	if (info->data == MAP_FAILED) goto fail_open;
	
	return info;
fail_open:
	return close(info->fd) != -1;
fail_start:
	return false;
}

bool mmap_close(mmap_info_t *info)
{
	// If didn't even open the file
	if (info->data == NULL) return true;
	
	// Not processing failures. It is more important to close the
	// file. In that case close errno will overwrite this errno.
	munmap(info->data, info->length);

	// Close the file.
	return close(info->fd) != -1;
}
