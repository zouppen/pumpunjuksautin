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
