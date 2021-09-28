#pragma once

// ASCII command interface

#include "../serial.h"

// Replace line ending (LF or CRLF) from the message with NUL
// character.
bool ascii_strip_line_ending(char *const buf, int const len);

// Process ASCII requests
bool ascii_interface(char *buf);
