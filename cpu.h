#pragma once

#include "hashmap.h"
#include "reversi.h"

typedef struct _TransposeTables {
    hashmap_t *upper;
    hashmap_t *lower;
} TransposeTables;

extern const int8_t SCORE_MATRIX[YSIZE][XSIZE];

extern void precompute_score_matrix(int16_t dst[YSIZE][UINT8_MAX],
                                    int8_t score_matrix[YSIZE][XSIZE]);

extern uint64_t search(Board *board, Validcoords *validcoords,
                       int16_t precompute_score_matrix[YSIZE][UINT8_MAX],
                       size_t depth);
