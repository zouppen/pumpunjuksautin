#pragma once

#include <sys/types.h>
#include <stdbool.h>

typedef enum {
	MMAP_MODE_READONLY,       // read-only access to the file and memory
	MMAP_MODE_WRITE,          // read and write access
	MMAP_MODE_VOLATILE_WRITE, // read-only access to the file but
				  // altering the data buffer is allowed
} mmap_mode_t;

typedef struct mmap_info {
	int fd;                // fd of the mmap'd file
	void *data;            // actual data
	off_t length;          // length of the data
} mmap_info_t;

/**
 * mmap() the given file at pathname and populates the variable
 * "info". Parameter 'mode' sets the access to the file and memory
 * area.  In case of error false is returned and errno is set. The
 * length of the mmap() is the file length.
 */
bool mmap_open(const char *pathname, mmap_mode_t mode, mmap_info_t *info);

/**
 * Closes the given mmapped file. Returns false on failure and sets errno.
 */
bool mmap_close(mmap_info_t *info);
