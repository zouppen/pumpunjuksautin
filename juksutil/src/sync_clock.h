#pragma once

// Synchronizes clock time of a device using Modbus
// protocol. Parameter real_time indicates if we synchronize current
// time or use supplied reference time.
void sync_clock_modbus(tzinfo_t const *tz, bool real_time, modbus_t *ctx);
