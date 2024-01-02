#pragma once

#include "reversi.h"


extern const int8_t SCORE_MATRIX[YSIZE][XSIZE];

extern void precompute_score_matrix(int16_t dst[YSIZE][UINT8_MAX],
                                    int8_t score_matrix[YSIZE][XSIZE]);

extern uint64_t search(Board *board, Validcoords *validcoords,
                       int16_t prepared_score_matrix[YSIZE][UINT8_MAX],
                       int depth);
