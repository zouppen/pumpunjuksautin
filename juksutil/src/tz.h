#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	int32_t gmtoff_now;
	uint32_t next_turn;
	int32_t gmtoff_turn;
} tzinfo_t;

// Populate given tzinfo with the next gmtoff change
bool tz_populate_tzinfo(char const *zone, tzinfo_t *info);
