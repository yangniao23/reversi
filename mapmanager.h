#pragma once

#include <stdint.h>

#include "reversi.h"

extern uint64_t coord_to_bit(int y, int x);
extern Validcoords *get_validcoords(Board *board);
extern uint64_t reverse_stones(Board *board, Validcoords *validcoords,
                               uint64_t put);