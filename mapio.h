#pragma once

#include "reversi.h"
#include "stdbool.h"

typedef struct {
    bool reset_flag;
    bool skip_flag;
    bool quit_flag;
    bool undo_flag;
} Flags;

extern uint64_t input_move(Board *board, Validcoords *validcoords,
                           Flags *flags);
extern void dump_bitmap(Board *board);
extern void dump_coords(uint64_t validcoords);
extern int read_map_file(const char *fname, Board *board);
extern int write_map_file(const char *fname, Board *board);