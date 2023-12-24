#pragma once

#include <stdint.h>

#include "reversi.h"

typedef enum { WHITE = -1, BLACK = 1 } Mode;
typedef struct {
    uint64_t white;
    uint64_t black;
    Mode mode;
} Board;

typedef struct {
    uint64_t coords;
    uint64_t reverse_stones[8];
} Validcoords;

extern Directionlist* check(const char map[YSIZE][XSIZE], int x0, int y0,
                            int mode);