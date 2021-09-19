#pragma once

// Modbus command interface

#include "serial.h"

// Return current Modbus station ID
uint8_t modbus_get_station_id(void);

// Processes given input and performs the Modbus operations in
// there. If returns 0, doesn't want to give any response.
buflen_t interface_modbus(char *buf, buflen_t len);
