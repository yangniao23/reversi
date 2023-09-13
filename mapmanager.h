#pragma once

#include "reversi.h"

extern Directionlist *check(const char map[YSIZE][XSIZE], int x0, int y0,
                            int mode);