#pragma once

// ASCII command interface

#include "serial.h"

// Process ASCII requests
bool interface_ascii(char *buf, buflen_t len);
