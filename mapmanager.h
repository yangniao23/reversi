#pragma once

#include <stdint.h>

#include "reversi.h"

#define CURRENT_MODE_BOARD(board) \
    *((board->mode == WHITE) ? &(board->white) : &(board->black))
#define OPPOSITE_MODE_BOARD(board) \
    *((board->mode == WHITE) ? &(board->black) : &(board->white))

extern uint64_t coord_to_bit(int y, int x);
extern Validcoords *get_validcoords(Board *board);
extern uint64_t reverse_stones(Board *board, Validcoords *validcoords,
                               uint64_t put);