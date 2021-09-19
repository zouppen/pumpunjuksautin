#pragma once

// Using GCC builtins for byte order swaps
// https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
#define bswap_16(x) __builtin_bswap16(x)
#define bswap_32(x) __builtin_bswap32(x)
#define bswap_64(x) __builtin_bswap64(x)
