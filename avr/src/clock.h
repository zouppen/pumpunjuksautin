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

// These variables are stored in EEPROM. Zone variables are signed
// despite of the data type. Parameters zone_now and zone_turn are in
// "gmtoff" format, i.e. seconds east to UTC, e.g. 3600 for
// UTC+1. Parameter zone_now is the currently observed gmtoffime
// zone. Parameter ts_turn defines when the clocks turn the next
// time. Parameter zone_turn is the gmtoff observed after that
// moment. In case of no known future DST changes, ts_turn must be 0.
extern uint32_t clock_ee_zone_now;
extern uint32_t clock_ee_ts_turn;
extern uint32_t clock_ee_zone_turn;

// Initialize clock using TIMER2
void clock_init(void);

// Set clock to given time. NB! Resets also internal tick
// counters. Timestamp epoch is UNIX.
void clock_set_time(time_t const ts_now);

// Sets time zone information from EEPROM- See clock_ee_* definitions
// above for how to set the data.
void clock_set_zones_from_eeprom(void);

// Test if clock is already set or is it running fake time
bool clock_is_set(void);

// Arm timer. Timeout unit is prescaler / F_CPU = 16µs and maximum
// delay of prescaler * COUNT_A / F_CPU = 4ms.
void clock_arm_timer(uint8_t timeout);

// Get currently observed UTC offset (time zone with DST adjustment,
// if any)
int32_t clock_get_gmtoff(void);
