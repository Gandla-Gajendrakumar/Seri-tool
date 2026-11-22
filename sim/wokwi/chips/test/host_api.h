// Host (PC) stand-in for the Wokwi chip API, so chips can be unit-tested natively.
// Force-included before the chip source: redirects printf into a capture buffer.
#pragma once
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
int host_printf(const char* fmt, ...);
#define printf host_printf
