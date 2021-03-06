#pragma once

/*
  Real-time clock

  Using dividers and counters to get 1 second from the clock
  frequency. You need to get F_CPU = CLOCK_PRESCALER * CLOCK_A *
  CLOCK_B. The variables are configurable in CMake.

  We use ATmega328P TIMER2 which is an 8-bit timer. So both CLOCK_A
  and CLOCK_B must be integers between 1 and 255. Prescaler can have
  value of 1, 8, 32, 64, 128, 256 or 1024 only.

  For example, when prescaler is 256 and CLOCK_A is 250, we generate
  Counter2 compare match A interrupt every 250x256 cycles which is 4
  ms when F_CPU is 16MHz. It is then counted to full seconds by an
  additional 250 counter CLOCK_B.

  Counter2 compare match A is used for the real-time clock, so
  compare match B interrupt is available for accuracy timing, having
  granularity of prescaler / F_CPU = 16µs.
*/

#include <time.h>
#include <stdbool.h>
#include "modbus_types.h"

// Initialize clock using TIMER2
void clock_init(void);

// Set clock to given time. NB! Resets also internal tick
// counters. Timestamp epoch is UNIX.
modbus_status_t clock_set_time_unix(time_t const ts_now);

// Test if clock is already set or is it running fake time
bool clock_is_set(void);

// Sets current gmtoff (time zone in seconds east to Greenwich).
modbus_status_t clock_set_gmtoff(int32_t gmtoff);

// Sets next clock turn timestamp. Timestamp has UNIX epoch.
modbus_status_t clock_set_next_turn(uint32_t ts);

// Sets gmtoff after next clock turn.
modbus_status_t clock_set_gmtoff_turn(int32_t gmtoff);

// Reads current time as UNIX timestamp
time_t clock_get_time_unix(void);

// Get currently observed UTC offset (time zone with DST adjustment,
// if any). May be different than the one set by clock_set_gmtoff() if
// clocks have already been turned.
int32_t clock_get_gmtoff(void);

// Read next clock turn as UNIX timestamp 
uint32_t clock_get_next_turn(void);

// Read next clock turn timestamp
int32_t clock_get_gmtoff_turn();

// Arm timer for interrupt TIMER2_COMPB_vect. Timeout unit is
// prescaler / F_CPU = 16µs and maximum delay of prescaler * COUNT_A /
// F_CPU = 4ms.
void clock_arm_timer(uint8_t timeout);
