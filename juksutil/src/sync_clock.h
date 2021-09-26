#pragma once

// Synchronizes clock time of a device. Parameter real_time indicates
// if we synchronize current time or use supplied reference
// time. Parameters dev, baud and slave are delivered to modbus
// library.
void sync_clock(tzinfo_t const *tz, bool real_time, char const *dev, int baud, int slave);
