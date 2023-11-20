#pragma once

#include "reversi.h"
#include "stdbool.h"

typedef struct {
    bool reset_flag;
    bool skip_flag;
    bool quit_flag;
} Flags;

extern Directionlist *input_move(char map[YSIZE][XSIZE],
                                 Validcoord *validcoords, int *x, int *y,
                                 Flags *flags);
extern void dump_map(char map[YSIZE][XSIZE]);
extern int read_map_file(const char *fname, char map[YSIZE][XSIZE]);
extern int write_map_file(const char *fname, char map[YSIZE][XSIZE]);