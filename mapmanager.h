#pragma once

#include <stdint.h>

#include "reversi.h"

#define CURRENT_MODE_BOARD(board) \
    *((board->mode == WHITE) ? &(board->white) : &(board->black))
#define OPPOSITE_MODE_BOARD(board) \
    *((board->mode == WHITE) ? &(board->black) : &(board->white))

extern uint64_t coord_to_bit(int y, int x);
extern char *bit_to_coord(char *coord, size_t length, uint64_t bit);
extern Validcoords *get_validcoords(Board *board);
extern uint64_t reverse_stones(Board *board, Validcoords *validcoords,
                               uint64_t put);
extern Board *copy_board(Board *dst, Board *src);