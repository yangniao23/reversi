#pragma once

#include "reversi.h"
#include "zhash.h"

typedef struct _TransposeTables {
    struct _TransposeTables *prev;
    struct ZHashTable *upper;
    struct ZHashTable *lower;
} TransposeTables;

extern const int8_t SCORE_MATRIX[YSIZE][XSIZE];

extern void precompute_score_matrix(int16_t dst[YSIZE][UINT8_MAX],
                                    int8_t score_matrix[YSIZE][XSIZE]);

extern TransposeTables *update_new_tables(TransposeTables *prev);
extern void free_tables(TransposeTables *tables);

extern uint64_t search(Board *board, Validcoords *validcoords,
                       int16_t precompute_score_matrix[YSIZE][UINT8_MAX],
                       size_t depth);
