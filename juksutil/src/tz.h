#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	time_t ref_time;
	int32_t gmtoff_now;
	uint64_t transition;
	int32_t gmtoff_after;
} tzinfo_t;

// Populate given tzinfo with the next gmtoff change
bool tz_populate_tzinfo(tzinfo_t *dest, char const *zone, int64_t now);

// Resolve time zone name. Results are indicative; with some
// non-standard symlinking it might return full pathname. If nothing
// is found, empty string is returned. The data is owned by the caller
// of the function.
GString *tz_name(char const *zone);

