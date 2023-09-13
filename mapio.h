#pragma once

#include "mapmanager.h"
#include "reversi.h"

extern Directionlist *input_move(Validcoord *validcoords, int *x, int *y);
extern void dump_map(char map[YSIZE][XSIZE]);
extern int read_map_file(const char *fname, char map[YSIZE][XSIZE]);
extern int write_map_file(const char *fname, char map[YSIZE][XSIZE]);