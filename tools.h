#pragma once

#include <stddef.h>
#include <stdint.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((y) > (x) ? (x) : (y))
#define UNUSED(x) ((void)(x))

extern void hexdump(const uint8_t *data, size_t len);

extern void flush(void);