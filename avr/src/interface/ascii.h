#pragma once

// ASCII command interface

#include "../serial.h"

// Process ASCII requests
bool ascii_interface(char *buf, buflen_t len);
